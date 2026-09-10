#include "lru_cache.hpp"
#include <gtest/gtest.h>

using fastcache::LRUCache;

TEST(LRUCache, InsertsAndReads) {
    LRUCache cache(3);
    EXPECT_TRUE(cache.insert_or_assign("a", "1"));
    EXPECT_TRUE(cache.insert_or_assign("b", "2"));

    std::string value;
    EXPECT_TRUE(cache.get("a", value));
    EXPECT_EQ(value, "1");
}

TEST(LRUCache, EvictsLeastRecentlyUsed) {
    LRUCache cache(2);
    cache.insert_or_assign("a", "1");
    cache.insert_or_assign("b", "2");

    std::string value;
    ASSERT_TRUE(cache.get("a", value));
    ASSERT_TRUE(cache.touch("a"));

    cache.insert_or_assign("c", "3");

    EXPECT_FALSE(cache.contains("b"));
    EXPECT_TRUE(cache.contains("a"));
    EXPECT_TRUE(cache.contains("c"));
}

TEST(LRUCache, UpdateDoesNotGrowSize) {
    LRUCache cache(2);
    cache.insert_or_assign("a", "1");
    cache.insert_or_assign("a", "2");

    EXPECT_EQ(cache.size(), 1u);
    std::string value;
    ASSERT_TRUE(cache.get("a", value));
    EXPECT_EQ(value, "2");
}
