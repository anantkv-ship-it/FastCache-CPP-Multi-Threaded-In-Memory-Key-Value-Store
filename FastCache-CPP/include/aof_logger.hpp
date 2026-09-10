#pragma once

#include <condition_variable>
#include <deque>
#include <fstream>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace fastcache {

class AOFLogger {
public:
    explicit AOFLogger(std::string path);
    ~AOFLogger();

    AOFLogger(const AOFLogger&) = delete;
    AOFLogger& operator=(const AOFLogger&) = delete;

    void append(std::string command);
    void stop();
    void replay(const std::function<void(const std::string&)>& callback);

private:
    void run();

    std::string path_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<std::string> queue_;
    bool stopping_ = false;
    std::thread worker_;
};

}  // namespace fastcache
