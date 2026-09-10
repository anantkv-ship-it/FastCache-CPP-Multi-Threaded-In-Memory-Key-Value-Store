#include "aof_logger.hpp"

#include <filesystem>
#include <functional>
#include <fstream>

namespace fastcache {

AOFLogger::AOFLogger(std::string path) : path_(std::move(path)), worker_(&AOFLogger::run, this) {}

AOFLogger::~AOFLogger() {
    stop();
}

void AOFLogger::append(std::string command) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) return;
        queue_.push_back(std::move(command));
    }
    cv_.notify_one();
}

void AOFLogger::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) return;
        stopping_ = true;
    }
    cv_.notify_one();
    if (worker_.joinable()) worker_.join();
}

void AOFLogger::run() {
    std::filesystem::path p(path_);
    if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());

    std::ofstream out(path_, std::ios::app);
    if (!out) return;

    while (true) {
        std::deque<std::string> local;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return stopping_ || !queue_.empty(); });

            local.swap(queue_);

            if (stopping_ && local.empty()) break;
        }

        for (const auto& command : local) {
            out << command << '\n';
            out.flush();
        }
    }
}

void AOFLogger::replay(const std::function<void(const std::string&)>& callback) {
    std::ifstream in(path_);
    if (!in) return;

    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty()) callback(line);
    }
}

}  // namespace fastcache
