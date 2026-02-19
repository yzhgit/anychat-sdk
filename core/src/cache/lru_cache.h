#pragma once

#include <list>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

namespace anychat::cache {

// Thread-safe generic LRU cache.
//
// Access pattern:
//   - get()      : shared_lock for the lookup, upgrades to unique_lock to
//                  splice the found node to the front.
//   - put()      : unique_lock (may evict + insert).
//   - remove()   : unique_lock.
//   - size()     : shared_lock.
//   - contains() : shared_lock.
//
// All iterators / references are internally managed; callers receive copies.
template <typename K, typename V>
class LruCache {
public:
    explicit LruCache(size_t capacity) : capacity_(capacity) {}

    // Returns a copy of the cached value, or std::nullopt on miss.
    // On hit the entry is promoted to most-recently-used.
    std::optional<V> get(const K& key);

    // Insert or update.  Evicts the least-recently-used entry when full.
    void put(const K& key, V value);

    void remove(const K& key);
    void clear();

    size_t size() const;
    bool   contains(const K& key) const;

private:
    size_t capacity_;

    mutable std::shared_mutex mu_;

    // Front = most-recently-used, back = least-recently-used.
    std::list<std::pair<K, V>> list_;
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> map_;
};

// ---------------------------------------------------------------------------
// Template implementation (must live in the header for non-explicit
// instantiation to work in downstream translation units).
// ---------------------------------------------------------------------------

template <typename K, typename V>
std::optional<V> LruCache<K, V>::get(const K& key) {
    // We need a unique_lock because splice() mutates the list structure.
    std::unique_lock<std::shared_mutex> lock(mu_);
    auto it = map_.find(key);
    if (it == map_.end()) return std::nullopt;
    // Promote to front (O(1) for std::list).
    list_.splice(list_.begin(), list_, it->second);
    return it->second->second;
}

template <typename K, typename V>
void LruCache<K, V>::put(const K& key, V value) {
    std::unique_lock<std::shared_mutex> lock(mu_);
    auto it = map_.find(key);
    if (it != map_.end()) {
        // Update existing entry and promote.
        it->second->second = std::move(value);
        list_.splice(list_.begin(), list_, it->second);
        return;
    }
    // Evict LRU if at capacity.
    if (list_.size() >= capacity_) {
        map_.erase(list_.back().first);
        list_.pop_back();
    }
    // Insert at front.
    list_.emplace_front(key, std::move(value));
    map_[key] = list_.begin();
}

template <typename K, typename V>
void LruCache<K, V>::remove(const K& key) {
    std::unique_lock<std::shared_mutex> lock(mu_);
    auto it = map_.find(key);
    if (it == map_.end()) return;
    list_.erase(it->second);
    map_.erase(it);
}

template <typename K, typename V>
void LruCache<K, V>::clear() {
    std::unique_lock<std::shared_mutex> lock(mu_);
    list_.clear();
    map_.clear();
}

template <typename K, typename V>
size_t LruCache<K, V>::size() const {
    std::shared_lock<std::shared_mutex> lock(mu_);
    return list_.size();
}

template <typename K, typename V>
bool LruCache<K, V>::contains(const K& key) const {
    std::shared_lock<std::shared_mutex> lock(mu_);
    return map_.count(key) > 0;
}

} // namespace anychat::cache
