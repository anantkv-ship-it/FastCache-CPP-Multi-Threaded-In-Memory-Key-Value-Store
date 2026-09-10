#include "kv_store.hpp"
#include "protocol.hpp"
#include <gtest/gtest.h>

using fastcache::KVStore;
using fastcache::Protocol;

TEST(Protocol, BasicCommands) {
    KVStore store(10);
    Protocol protocol(store);

    EXPECT_EQ(protocol.execute("PING"), "PONG");
    EXPECT_EQ(protocol.execute("SET a 42"), "OK");
    EXPECT_EQ(protocol.execute("GET a"), "42");
    EXPECT_EQ(protocol.execute("DEL a"), "1");
    EXPECT_EQ(protocol.execute("GET a"), "(nil)");
}

TEST(Protocol, InvalidCommands) {
    KVStore store(10);
    Protocol protocol(store);

    EXPECT_EQ(protocol.execute("GET"), "ERR usage: GET key");
    EXPECT_EQ(protocol.execute("SET a"), "ERR usage: SET key value");
    EXPECT_EQ(protocol.execute("WHAT a"), "ERR unknown command");
}
