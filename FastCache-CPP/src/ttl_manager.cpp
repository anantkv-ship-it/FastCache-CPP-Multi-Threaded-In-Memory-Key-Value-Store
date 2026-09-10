#include "ttl_manager.hpp"

#include <unordered_map>

namespace fastcache {

TTLManager::TTLManager(ExpireCallback callback)
    : callback_(std::move(callback)), worker_(&TTLManager::run, this) {}

TTLManager::~TTLManager() {
    stop();
}

void TTLManager::schedule(const std::string& key, std::chrono::seconds ttl) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto generation = next_generation_++;
        generations_[key] = generation;
        heap_.push({Clock::now() + ttl, key, generation});
    }
    cv_.notify_one();
}

void TTLManager::cancel(const std::string& key) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        generations_.erase(key);
    }
    cv_.notify_one();
}

void TTLManager::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) return;
        stopping_ = true;
    }
    cv_.notify_one();
    if (worker_.joinable()) worker_.join();
}

void TTLManager::run() {
    std::unique_lock<std::mutex> lock(mutex_);

    while (!stopping_) {
        if (heap_.empty()) {
            cv_.wait(lock, [this] { return stopping_ || !heap_.empty(); });
            continue;
        }

        const auto deadline = heap_.top().when;
        if (cv_.wait_until(lock, deadline, [this, deadline] {
                return stopping_ || heap_.top().when < deadline;
            })) {
            continue;
        }

        const Entry entry = heap_.top();
        heap_.pop();

        auto it = generations_.find(entry.key);
        if (it == generations_.end() || it->second != entry.generation) {
            continue;
        }

        generations_.erase(it);

        lock.unlock();
        callback_(entry.key, entry.generation);
        lock.lock();
    }
}

}  // namespace fastcache
