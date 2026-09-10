#include "lru_cache.hpp"

#include <utility>

namespace fastcache {

LRUCache::LRUCache(std::size_t capacity) : capacity_(capacity) {
    index_.reserve(capacity_);
}

LRUCache::~LRUCache() {
    clear();
}

void LRUCache::clear() noexcept {
    Node* cur = head_;
    while (cur) {
        Node* next = cur->next;
        delete cur;
        cur = next;
    }
    head_ = tail_ = nullptr;
    index_.clear();
}

void LRUCache::unlink(Node* node) noexcept {
    if (node->prev) node->prev->next = node->next;
    else head_ = node->next;

    if (node->next) node->next->prev = node->prev;
    else tail_ = node->prev;

    node->prev = node->next = nullptr;
}

void LRUCache::push_front(Node* node) noexcept {
    node->prev = nullptr;
    node->next = head_;

    if (head_) head_->prev = node;
    else tail_ = node;

    head_ = node;
}

bool LRUCache::get(const std::string& key, std::string& value) const {
    auto it = index_.find(key);
    if (it == index_.end()) return false;
    value = it->second->value;
    return true;
}

bool LRUCache::contains(const std::string& key) const {
    return index_.find(key) != index_.end();
}

bool LRUCache::touch(const std::string& key) {
    auto it = index_.find(key);
    if (it == index_.end()) return false;

    Node* node = it->second;
    if (node != head_) {
        unlink(node);
        push_front(node);
    }
    return true;
}

bool LRUCache::insert_or_assign(const std::string& key, const std::string& value) {
    auto it = index_.find(key);

    if (it != index_.end()) {
        it->second->value = value;
        touch(key);
        return false;
    }

    auto* node = new Node{key, value};
    index_[key] = node;
    push_front(node);

    if (index_.size() > capacity_) {
        std::string ignored_key;
        std::string ignored_value;
        pop_lru(ignored_key, ignored_value);
    }
    return true;
}

bool LRUCache::erase_without_external_lock(const std::string& key) {
    auto it = index_.find(key);
    if (it == index_.end()) return false;

    Node* node = it->second;
    unlink(node);
    index_.erase(it);
    delete node;
    return true;
}

bool LRUCache::erase(const std::string& key) {
    return erase_without_external_lock(key);
}

bool LRUCache::pop_lru(std::string& key, std::string& value) {
    if (!tail_) return false;

    Node* node = tail_;
    key = node->key;
    value = node->value;

    unlink(node);
    index_.erase(node->key);
    delete node;
    return true;
}

std::size_t LRUCache::size() const noexcept {
    return index_.size();
}

std::size_t LRUCache::capacity() const noexcept {
    return capacity_;
}

}  // namespace fastcache
