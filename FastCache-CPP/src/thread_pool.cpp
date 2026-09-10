#include "thread_pool.hpp"

#include <stdexcept>

namespace fastcache {

ThreadPool::ThreadPool(std::size_t workers) {
    if (workers == 0) throw std::invalid_argument("ThreadPool requires at least one worker");

    workers_.reserve(workers);
    for (std::size_t i = 0; i < workers; ++i) {
        workers_.emplace_back(&ThreadPool::worker_loop, this);
    }
}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) throw std::runtime_error("enqueue on stopped ThreadPool");
        tasks_.push(std::move(task));
    }
    cv_.notify_one();
}

void ThreadPool::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) return;
        stopping_ = true;
    }

    cv_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) worker.join();
    }
}

void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });

            if (stopping_ && tasks_.empty()) return;

            task = std::move(tasks_.front());
            tasks_.pop();
        }

        task();
    }
}

}  // namespace fastcache
