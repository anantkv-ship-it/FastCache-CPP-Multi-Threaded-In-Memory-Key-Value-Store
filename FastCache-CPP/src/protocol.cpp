#include "protocol.hpp"

#include <chrono>
#include <sstream>

namespace fastcache {

Protocol::Protocol(KVStore& store) : store_(store) {}

std::string Protocol::execute(const std::string& line) {
    std::istringstream iss(line);
    std::string op;
    if (!(iss >> op)) return "ERR empty command";

    if (op == "PING") return "PONG";

    if (op == "GET") {
        std::string key;
        if (!(iss >> key)) return "ERR usage: GET key";
        auto value = store_.get(key);
        return value ? *value : "(nil)";
    }

    if (op == "SET") {
        std::string key;
        std::string value;
        if (!(iss >> key >> value)) return "ERR usage: SET key value";

        std::string token;
        long long seconds;
        if (iss >> token) {
            if (token != "EX" || !(iss >> seconds) || seconds <= 0) {
                return "ERR usage: SET key value [EX seconds]";
            }
            return store_.set(key, value, std::chrono::seconds(seconds))
                       ? "OK"
                       : "ERR set failed";
        }

        return store_.set(key, value) ? "OK" : "ERR set failed";
    }

    if (op == "DEL") {
        std::string key;
        if (!(iss >> key)) return "ERR usage: DEL key";
        return store_.del(key) ? "1" : "0";
    }

    if (op == "EXPIRE") {
        std::string key;
        long long seconds;
        if (!(iss >> key >> seconds) || seconds <= 0) {
            return "ERR usage: EXPIRE key seconds";
        }
        return store_.expire(key, std::chrono::seconds(seconds)) ? "1" : "0";
    }

    if (op == "STATS") {
        const auto s = store_.stats();
        std::ostringstream out;
        out << "size=" << s.size
            << " capacity=" << s.capacity
            << " gets=" << s.gets
            << " hits=" << s.hits
            << " sets=" << s.sets
            << " deletes=" << s.deletes;
        return out.str();
    }

    if (op == "QUIT") return "BYE";

    return "ERR unknown command";
}

}  // namespace fastcache
