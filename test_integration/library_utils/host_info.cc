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

#include "host_info.h"

HostInfo::HostInfo(const std::string& host, int port, HOST_STATE state, bool is_writer, std::shared_ptr<HostAvailabilityStrategy> host_availability_strategy, uint64_t weight) :
    host { std::move(host) },
    port { port },
    host_state { state },
    is_writer { is_writer },
    host_availability_strategy{ std::move(host_availability_strategy) },
    weight { weight }
{
}

/**
 * Returns the host.
 *
 * @return the host
 */
std::string HostInfo::GetHost() const {
    return host;
}

/**
 * Returns the port.
 *
 * @return the port
 */
int HostInfo::GetPort() const {
    return port;
}

/**
 * Returns the weight
 *
 * @return the weight
 */
uint64_t HostInfo::GetWeight() const {
    return weight;
}

/**
 * Returns a host:port representation of this host.
 *
 * @return the host:port representation of this host
 */
std::string HostInfo::GetHostPortPair() const {
    return GetHost() + host_port_separator + std::to_string(GetPort());
}

bool HostInfo::EqualHostPortPair(const HostInfo& hi) const {
    return GetHostPortPair() == hi.GetHostPortPair();
}

HOST_STATE HostInfo::GetHostState() const {
    return host_state;
}

void HostInfo::SetHostState(HOST_STATE state) {
    host_state = state;
}

bool HostInfo::IsHostUp() const {
    return host_state == UP;
}

bool HostInfo::IsHostDown() const {
    return host_state == DOWN;
}

bool HostInfo::IsHostWriter() const {
    return is_writer;
}

void HostInfo::MarkAsWriter(bool writer) {
    is_writer = writer;
}

std::shared_ptr<HostAvailabilityStrategy> HostInfo::GetHostAvailabilityStrategy() const {
    return host_availability_strategy;
}

void HostInfo::SetHostAvailabilityStrategy(std::shared_ptr<HostAvailabilityStrategy> new_host_availability_strategy) {
    host_availability_strategy = std::move(new_host_availability_strategy);
}
