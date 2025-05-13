// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "sliding_cache_map.h"

#include "round_robin_host_selector.h"
#include "host_info.h"

template <typename K, typename V>
void SlidingCacheMap<K, V>::Put(const K& key, const V& value) {
    Put(key, value, DEFAULT_EXPIRATION_SEC);
}

template <typename K, typename V>
void SlidingCacheMap<K, V>::Put(const K& key, const V& value, int sec_ttl) {
    std::lock_guard<std::mutex> lock(cache_lock);
    std::chrono::steady_clock::time_point expiry_time =
        std::chrono::steady_clock::now() + std::chrono::seconds(sec_ttl);
    cache[key] = CacheEntry{value, expiry_time, sec_ttl};
}

template <typename K, typename V>
void SlidingCacheMap<K, V>::putIfAbsent(const K &key, const V &value) {
    putIfAbsent(key, value, DEFAULT_EXPIRATION_SEC);
}

template <typename K, typename V>
void SlidingCacheMap<K, V>::putIfAbsent(const K &key, const V &value, int sec_ttl) {
    std::lock_guard<std::mutex> lock(cache_lock);
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    if (auto itr = cache.find(key); itr != cache.end()) {
        CacheEntry& entry = itr->second;
        // Already in cache & is not expired
        if (entry.expiry > now) {
            // Update TTL & Return value
            entry.expiry = now + std::chrono::seconds(entry.sec_ttl);
            return;
        }
    }
    // Either not in cache or is expired, put new into cache
    std::chrono::steady_clock::time_point expiry_time =
        std::chrono::steady_clock::now() + std::chrono::seconds(sec_ttl);
    cache[key] = CacheEntry{value, expiry_time, sec_ttl};
}

template <typename K, typename V>
V SlidingCacheMap<K, V>::Get(const K& key) {
    std::lock_guard<std::mutex> lock(cache_lock);
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    if (auto itr = cache.find(key); itr != cache.end()) {
        CacheEntry& entry = itr->second;
        if (entry.expiry > now) {
            // Update TTL & Return value
            entry.expiry = now + std::chrono::seconds(entry.sec_ttl);
            return entry.value;
        }
        // Expired, remove from cache
        cache.erase(itr);
    }
    return {};
}

template <typename K, typename V>
bool SlidingCacheMap<K, V>::Find(const K& key) {
    std::lock_guard<std::mutex> lock(cache_lock);
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    if (auto itr = cache.find(key); itr != cache.end()) {
        CacheEntry& entry = itr->second;
        if (entry.expiry > now) {
            // Update TTL & Return found
            entry.expiry = now + std::chrono::seconds(entry.sec_ttl);
            return true;
        }
        // Expired, remove from cache
        cache.erase(itr);
    }
    return false;
}

template <typename K, typename V>
unsigned int SlidingCacheMap<K, V>::Size() {
    std::lock_guard<std::mutex> lock(cache_lock);
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    for (auto itr = cache.begin(); itr != cache.end();) {
        if (itr->second.expiry < now) {
            itr = cache.erase(itr);
        } else {
            itr++;
        }
    }
    return cache.size();
}

template <typename K, typename V>
void SlidingCacheMap<K, V>::Clear() {
    std::lock_guard<std::mutex> lock(cache_lock);
    cache.clear();
}

// Explicit Template Instantiations
template class SlidingCacheMap<std::string, std::shared_ptr<round_robin_property::RoundRobinClusterInfo>>;
template class SlidingCacheMap<std::string, std::string>;
template class SlidingCacheMap<std::string, std::vector<HostInfo>>;
