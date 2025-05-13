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

#ifndef ROUNDROBIN_HOST_SELECTOR_H_
#define ROUNDROBIN_HOST_SELECTOR_H_

#include <mutex>
#include <vector>

#include "host_info.h"
#include "sliding_cache_map.h"
#include "host_selector.h"
#include "round_robin_property.h"

class RoundRobinHostSelector: public HostSelector {
public:
    HostInfo GetHost(std::vector<HostInfo> hosts, bool is_writer,
        std::unordered_map<std::string, std::string> properties) override;
    static void SetRoundRobinWeight(std::vector<HostInfo> hosts, 
        std::unordered_map<std::string, std::string>& properties);
    static void ClearCache();

private:
    const int DEFAULT_WEIGHT = 1;
    const int NO_HOST_IDX = -1;

    static std::mutex cache_mutex;
    static SlidingCacheMap<std::string, std::shared_ptr<round_robin_property::RoundRobinClusterInfo>> round_robin_cache;

    virtual int convert_to_int(const std::string& str);

    virtual bool check_prop_change(const std::string& value, const std::string& prop_name,
        const std::unordered_map<std::string, std::string>& props);
    virtual void create_cache_entries(const std::vector<HostInfo>& hosts,
        const std::unordered_map<std::string, std::string>& props);
    virtual void update_cache(const std::vector<HostInfo>& hosts,
        const std::shared_ptr<round_robin_property::RoundRobinClusterInfo>& cluster_info);
    virtual void update_props_default_weight(
        const std::shared_ptr<round_robin_property::RoundRobinClusterInfo>& info,
        const std::unordered_map<std::string, std::string>& props);
    virtual void update_props_host_weight(
        const std::shared_ptr<round_robin_property::RoundRobinClusterInfo>& info,
        const std::unordered_map<std::string, std::string>& props);
};

#endif // ROUNDROBIN_HOST_SELECTOR_H_
