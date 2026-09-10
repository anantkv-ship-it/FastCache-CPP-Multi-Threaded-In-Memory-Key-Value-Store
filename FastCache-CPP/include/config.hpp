#pragma once

#include <cstddef>
#include <string>

namespace fastcache {

struct Config {
    int port = 8080;
    std::size_t worker_threads = 4;
    std::size_t max_items = 100000;
    std::string aof_path = "data/appendonly.aof";
};

Config parse_config(int argc, char** argv);

}  // namespace fastcache
