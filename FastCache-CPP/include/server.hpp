#pragma once

#include "config.hpp"
#include "kv_store.hpp"
#include "protocol.hpp"
#include "thread_pool.hpp"

#include <atomic>
#include <memory>

namespace fastcache {

class Server {
public:
    explicit Server(Config config);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void run();
    void stop();

private:
    void handle_client(int client_fd);
    static void close_socket(int fd);

    Config config_;
    KVStore store_;
    Protocol protocol_;
    ThreadPool pool_;
    int server_fd_ = -1;
    std::atomic<bool> stopping_{false};
};

}  // namespace fastcache
