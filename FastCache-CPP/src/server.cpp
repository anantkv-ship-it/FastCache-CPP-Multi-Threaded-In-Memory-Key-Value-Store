#include "server.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace fastcache {

Server::Server(Config config)
    : config_(std::move(config)),
      store_(config_.max_items, config_.aof_path),
      protocol_(store_),
      pool_(config_.worker_threads) {}

Server::~Server() {
    stop();
}

void Server::close_socket(int fd) {
    if (fd >= 0) ::close(fd);
}

void Server::run() {
    server_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) throw std::runtime_error("socket() failed");

    int reuse = 1;
    ::setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(static_cast<std::uint16_t>(config_.port));

    if (::bind(server_fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        close_socket(server_fd_);
        server_fd_ = -1;
        throw std::runtime_error("bind() failed: " + std::string(std::strerror(errno)));
    }

    if (::listen(server_fd_, 128) < 0) {
        close_socket(server_fd_);
        server_fd_ = -1;
        throw std::runtime_error("listen() failed");
    }

    std::cout << "FastCache listening on port " << config_.port
              << " with " << config_.worker_threads << " workers\n";

    while (!stopping_) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        const int client_fd =
            ::accept(server_fd_, reinterpret_cast<sockaddr*>(&client_addr), &len);

        if (client_fd < 0) {
            if (stopping_) break;
            if (errno == EINTR) continue;
            std::cerr << "accept() failed: " << std::strerror(errno) << '\n';
            continue;
        }

        pool_.enqueue([this, client_fd] { handle_client(client_fd); });
    }
}

void Server::handle_client(int client_fd) {
    std::string buffer;
    char chunk[4096];

    while (!stopping_) {
        const ssize_t n = ::recv(client_fd, chunk, sizeof(chunk), 0);
        if (n == 0) break;
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }

        buffer.append(chunk, static_cast<std::size_t>(n));

        std::size_t newline;
        while ((newline = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, newline);
            buffer.erase(0, newline + 1);

            if (!line.empty() && line.back() == '\r') line.pop_back();

            const std::string response = protocol_.execute(line);
            const std::string wire = response + "\r\n";

            std::size_t sent = 0;
            while (sent < wire.size()) {
                const ssize_t m = ::send(client_fd, wire.data() + sent,
                                         wire.size() - sent, MSG_NOSIGNAL);
                if (m <= 0) {
                    sent = wire.size();
                    break;
                }
                sent += static_cast<std::size_t>(m);
            }

            if (line == "QUIT") {
                close_socket(client_fd);
                return;
            }
        }
    }

    close_socket(client_fd);
}

void Server::stop() {
    if (stopping_.exchange(true)) return;

    if (server_fd_ >= 0) {
        ::shutdown(server_fd_, SHUT_RDWR);
        close_socket(server_fd_);
        server_fd_ = -1;
    }

    pool_.stop();
}

}  // namespace fastcache
