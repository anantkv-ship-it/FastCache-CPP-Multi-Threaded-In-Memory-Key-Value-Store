#pragma once

#include "aof_logger.hpp"
#include "lru_cache.hpp"
#include "ttl_manager.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace fastcache {

struct StoreStats {
    std::size_t size = 0;
    std::size_t capacity = 0;
    std::uint64_t gets = 0;
    std::uint64_t hits = 0;
    std::uint64_t sets = 0;
    std::uint64_t deletes = 0;
};

class KVStore {
public:
    explicit KVStore(std::size_t capacity, const std::string& aof_path = "");
    ~KVStore();

    KVStore(const KVStore&) = delete;
    KVStore& operator=(const KVStore&) = delete;

    bool set(const std::string& key, const std::string& value);
    bool set(const std::string& key, const std::string& value, std::chrono::seconds ttl);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    bool expire(const std::string& key, std::chrono::seconds ttl);

    StoreStats stats() const;
    void replay_aof();

private:
    bool set_internal(const std::string& key, const std::string& value,
                      std::optional<std::chrono::seconds> ttl, bool log);
    void expire_callback(const std::string& key, std::uint64_t generation);

    mutable std::shared_mutex mutex_;
    LRUCache cache_;
    std::unordered_map<std::string, std::uint64_t> ttl_generations_;

    std::unique_ptr<AOFLogger> aof_;
    std::unique_ptr<TTLManager> ttl_manager_;

    std::atomic<std::uint64_t> gets_{0};
    std::atomic<std::uint64_t> hits_{0};
    std::atomic<std::uint64_t> sets_{0};
    std::atomic<std::uint64_t> deletes_{0};
};

}  // namespace fastcache
