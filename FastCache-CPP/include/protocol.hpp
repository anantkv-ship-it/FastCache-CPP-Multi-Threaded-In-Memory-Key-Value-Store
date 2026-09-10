#pragma once

#include "kv_store.hpp"

#include <string>

namespace fastcache {

class Protocol {
public:
    explicit Protocol(KVStore& store);

    std::string execute(const std::string& line);

private:
    KVStore& store_;
};

}  // namespace fastcache
