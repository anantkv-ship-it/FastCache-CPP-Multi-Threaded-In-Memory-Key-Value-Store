cmake_minimum_required(VERSION 3.14)
project(FastCacheCPP VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FASTCACHE_BUILD_TESTS "Build GoogleTest tests" ON)

find_package(Threads REQUIRED)

add_library(fastcache_core
    src/aof_logger.cpp
    src/config.cpp
    src/kv_store.cpp
    src/lru_cache.cpp
    src/protocol.cpp
    src/server.cpp
    src/thread_pool.cpp
    src/ttl_manager.cpp
)

target_include_directories(fastcache_core PUBLIC include)
target_link_libraries(fastcache_core PUBLIC Threads::Threads)

if(UNIX)
    target_compile_definitions(fastcache_core PRIVATE _POSIX_C_SOURCE=200809L)
endif()

target_compile_options(fastcache_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

add_executable(fastcache_server src/main.cpp)
target_link_libraries(fastcache_server PRIVATE fastcache_core)

if(FASTCACHE_BUILD_TESTS)
    enable_testing()
    include(FetchContent)

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(fastcache_tests
        tests/test_kv_store.cpp
        tests/test_lru.cpp
        tests/test_protocol.cpp
    )
    target_link_libraries(fastcache_tests PRIVATE fastcache_core GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(fastcache_tests)
endif()
