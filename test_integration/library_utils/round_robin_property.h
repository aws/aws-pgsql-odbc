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

#ifndef ROUND_ROBIN_PROPERTY_H
#define ROUND_ROBIN_PROPERTY_H

#include <memory>
#include <string>
#include <unordered_map>

#include "host_info.h"

namespace round_robin_property {
    const std::string HOST_WEIGHT_KEY = "round_robin_host_weight_pairs";
    const std::string DEFAULT_WEIGHT_KEY = "round_robin_default_weight";

    using RoundRobinClusterInfo = struct RoundRobinClusterInfo {
        std::shared_ptr<HostInfo> last_host;
        std::unordered_map<std::string, int> cluster_weight_map;
        int default_weight = 1;
        int weigth_counter = 0;
        std::string last_default_weight_str;
        std::string last_host_weight_str;
    };
};

#endif
