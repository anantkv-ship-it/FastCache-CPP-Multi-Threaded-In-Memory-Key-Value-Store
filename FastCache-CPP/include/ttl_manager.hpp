#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace fastcache {

class TTLManager {
public:
    using Clock = std::chrono::steady_clock;
    using ExpireCallback = std::function<void(const std::string&, std::uint64_t)>;

    explicit TTLManager(ExpireCallback callback);
    ~TTLManager();

    void schedule(const std::string& key, std::chrono::seconds ttl);
    void cancel(const std::string& key);
    void stop();

private:
    struct Entry {
        Clock::time_point when;
        std::string key;
        std::uint64_t generation;
    };

    struct Compare {
        bool operator()(const Entry& a, const Entry& b) const {
            return a.when > b.when;
        }
    };

    void run();

    ExpireCallback callback_;
    std::priority_queue<Entry, std::vector<Entry>, Compare> heap_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::uint64_t next_generation_ = 1;
    std::unordered_map<std::string, std::uint64_t> generations_;
    bool stopping_ = false;
    std::thread worker_;
};

}  // namespace fastcache
