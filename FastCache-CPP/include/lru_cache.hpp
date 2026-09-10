#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>

namespace fastcache {

class LRUCache {
public:
    struct Node {
        std::string key;
        std::string value;
        Node* prev = nullptr;
        Node* next = nullptr;
    };

    explicit LRUCache(std::size_t capacity);
    ~LRUCache();

    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;

    bool get(const std::string& key, std::string& value) const;
    bool insert_or_assign(const std::string& key, const std::string& value);
    bool erase(const std::string& key);
    bool touch(const std::string& key);
    bool contains(const std::string& key) const;

    std::size_t size() const noexcept;
    std::size_t capacity() const noexcept;

    // Must be called only while the owning store lock is held.
    bool erase_without_external_lock(const std::string& key);
    bool pop_lru(std::string& key, std::string& value);

private:
    void unlink(Node* node) noexcept;
    void push_front(Node* node) noexcept;
    void clear() noexcept;

    std::size_t capacity_;
    std::unordered_map<std::string, Node*> index_;
    Node* head_ = nullptr;
    Node* tail_ = nullptr;
};

}  // namespace fastcache
