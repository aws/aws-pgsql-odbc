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

#ifndef SLIDING_CACHE_MAP_H_
#define SLIDING_CACHE_MAP_H_

#include <chrono>
#include <mutex>
#include <unordered_map>

template <typename K, typename V>
class SlidingCacheMap {
public:
    SlidingCacheMap() = default;

    void Put(const K& key, const V& value);
    void Put(const K& key, const V& value, int sec_ttl);
    void putIfAbsent(const K& key, const V& value);
    void putIfAbsent(const K& key, const V& value, int sec_ttl);
    V Get(const K& key);
    bool Find(const K& key);
    unsigned int Size();
    void Clear();

private:
    struct CacheEntry {
        V value;
        std::chrono::steady_clock::time_point expiry;
        int sec_ttl;
    };
    const int DEFAULT_EXPIRATION_SEC = 600; // 600s = 10m
    std::unordered_map<K, CacheEntry> cache;
    std::mutex cache_lock;
};

#endif // SLIDING_CACHE_MAP_H_
