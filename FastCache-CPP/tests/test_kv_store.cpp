#include "kv_store.hpp"
#include <gtest/gtest.h>

#include <chrono>
#include <thread>

using fastcache::KVStore;

TEST(KVStore, SetGetDelete) {
    KVStore store(10);
    EXPECT_TRUE(store.set("name", "alice"));
    EXPECT_EQ(store.get("name").value_or(""), "alice");
    EXPECT_TRUE(store.del("name"));
    EXPECT_FALSE(store.get("name").has_value());
}

TEST(KVStore, ExpiresKey) {
    KVStore store(10);
    EXPECT_TRUE(store.set("temp", "value", std::chrono::seconds(1)));
    EXPECT_EQ(store.get("temp").value_or(""), "value");

    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    EXPECT_FALSE(store.get("temp").has_value());
}

TEST(KVStore, ConcurrentAccess) {
    KVStore store(1000);

    constexpr int threads = 8;
    constexpr int operations = 500;

    std::vector<std::thread> workers;
    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&store, t] {
            for (int i = 0; i < operations; ++i) {
                const std::string key = "k" + std::to_string((t * operations + i) % 100);
                store.set(key, std::to_string(i));
                (void)store.get(key);
            }
        });
    }

    for (auto& worker : workers) worker.join();

    EXPECT_LE(store.stats().size, 100u);
}
