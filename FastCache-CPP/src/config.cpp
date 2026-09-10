#include "config.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fastcache {

namespace {

void require_value(int i, int argc, const char* option) {
    if (i + 1 >= argc) {
        throw std::invalid_argument(std::string("Missing value for ") + option);
    }
}

}  // namespace

Config parse_config(int argc, char** argv) {
    Config cfg;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--help") {
            std::cout
                << "Usage: fastcache_server [options]\\n"
                << "  --port N          TCP port (default 8080)\\n"
                << "  --threads N       worker threads (default 4)\\n"
                << "  --max-items N     maximum cached keys (default 100000)\\n"
                << "  --aof PATH        AOF path (default data/appendonly.aof)\\n";
            std::exit(0);
        }

        require_value(i, argc, arg.c_str());
        const std::string value = argv[++i];

        try {
            if (arg == "--port") {
                cfg.port = std::stoi(value);
                if (cfg.port < 1 || cfg.port > 65535) throw std::invalid_argument("port");
            } else if (arg == "--threads") {
                cfg.worker_threads = std::stoull(value);
                if (cfg.worker_threads == 0) throw std::invalid_argument("threads");
            } else if (arg == "--max-items") {
                cfg.max_items = std::stoull(value);
                if (cfg.max_items == 0) throw std::invalid_argument("max-items");
            } else if (arg == "--aof") {
                cfg.aof_path = value;
            } else {
                throw std::invalid_argument("unknown option");
            }
        } catch (const std::exception&) {
            throw std::invalid_argument("Invalid option/value: " + arg + " " + value);
        }
    }

    return cfg;
}

}  // namespace fastcache
