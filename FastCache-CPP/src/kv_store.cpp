#include "kv_store.hpp"

#include <sstream>

namespace fastcache {

KVStore::KVStore(std::size_t capacity, const std::string& aof_path)
    : cache_(capacity) {
    ttl_manager_ = std::make_unique<TTLManager>(
        [this](const std::string& key, std::uint64_t generation) {
            expire_callback(key, generation);
        });

    if (!aof_path.empty()) {
        aof_ = std::make_unique<AOFLogger>(aof_path);
        replay_aof();
    }
}

KVStore::~KVStore() {
    ttl_manager_->stop();
    if (aof_) aof_->stop();
}

bool KVStore::set_internal(const std::string& key, const std::string& value,
                           std::optional<std::chrono::seconds> ttl, bool log) {
    if (key.empty()) return false;

    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        cache_.insert_or_assign(key, value);
    }

    sets_.fetch_add(1, std::memory_order_relaxed);

    if (ttl) {
        ttl_manager_->schedule(key, *ttl);
    } else {
        ttl_manager_->cancel(key);
    }

    if (log && aof_) {
        std::ostringstream cmd;
        cmd << "SET " << key << ' ' << value;
        if (ttl) cmd << " EX " << ttl->count();
        aof_->append(cmd.str());
    }

    return true;
}

bool KVStore::set(const std::string& key, const std::string& value) {
    return set_internal(key, value, std::nullopt, true);
}

bool KVStore::set(const std::string& key, const std::string& value,
                  std::chrono::seconds ttl) {
    if (ttl.count() <= 0) return del(key);
    return set_internal(key, value, ttl, true);
}

std::optional<std::string> KVStore::get(const std::string& key) {
    gets_.fetch_add(1, std::memory_order_relaxed);

    std::string value;
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        if (!cache_.get(key, value)) return std::nullopt;
    }

    hits_.fetch_add(1, std::memory_order_relaxed);

    // Recency is mutable state, so touching the LRU requires exclusive access.
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        cache_.touch(key);
    }

    return value;
}

bool KVStore::del(const std::string& key) {
    bool removed = false;

    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        removed = cache_.erase(key);
        ttl_generations_.erase(key);
    }

    if (removed) {
        deletes_.fetch_add(1, std::memory_order_relaxed);
        ttl_manager_->cancel(key);
        if (aof_) aof_->append("DEL " + key);
    }

    return removed;
}

bool KVStore::expire(const std::string& key, std::chrono::seconds ttl) {
    if (ttl.count() <= 0) return del(key);

    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        if (!cache_.contains(key)) return false;
    }

    ttl_manager_->schedule(key, ttl);

    if (aof_) {
        std::ostringstream cmd;
        cmd << "EXPIRE " << key << ' ' << ttl.count();
        aof_->append(cmd.str());
    }
    return true;
}

void KVStore::expire_callback(const std::string& key, std::uint64_t generation) {
    // Generation validation is handled by TTLManager; this callback only
    // removes the key if it still exists.
    (void)generation;
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (cache_.erase(key)) {
        deletes_.fetch_add(1, std::memory_order_relaxed);
    }
}

StoreStats KVStore::stats() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return {
        cache_.size(),
        cache_.capacity(),
        gets_.load(std::memory_order_relaxed),
        hits_.load(std::memory_order_relaxed),
        sets_.load(std::memory_order_relaxed),
        deletes_.load(std::memory_order_relaxed)
    };
}

void KVStore::replay_aof() {
    if (!aof_) return;

    aof_->replay([this](const std::string& command) {
        std::istringstream iss(command);
        std::string op, key, value, ex;
        long long seconds = 0;

        if (!(iss >> op >> key)) return;

        if (op == "SET") {
            if (!(iss >> value)) return;
            if (iss >> ex >> seconds && ex == "EX") {
                set_internal(key, value, std::chrono::seconds(seconds), false);
            } else {
                set_internal(key, value, std::nullopt, false);
            }
        } else if (op == "DEL") {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            cache_.erase(key);
        } else if (op == "EXPIRE") {
            if (iss >> seconds) {
                std::shared_lock<std::shared_mutex> lock(mutex_);
                if (cache_.contains(key)) {
                    lock.unlock();
                    ttl_manager_->schedule(key, std::chrono::seconds(seconds));
                }
            }
        }
    });
}

}  // namespace fastcache
