#include "config.hpp"
#include "server.hpp"

#include <csignal>
#include <iostream>
#include <memory>

namespace {
fastcache::Server* g_server = nullptr;

void signal_handler(int) {
    if (g_server) g_server->stop();
}
}  // namespace

int main(int argc, char** argv) {
    try {
        auto config = fastcache::parse_config(argc, argv);
        fastcache::Server server(std::move(config));
        g_server = &server;

        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        server.run();
        g_server = nullptr;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "fatal: " << ex.what() << '\n';
        return 1;
    }
}
