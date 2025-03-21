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

#ifndef CONNECTION_STRING_BUILDER_H_
#define CONNECTION_STRING_BUILDER_H_

#include "integration_test_utils.h"

#include <string>

class ConnectionStringBuilder {
   public:
    ConnectionStringBuilder(const std::string& dsn, const std::string& server, int port) {
        length += sprintf(conn_in, "DSN=%s;SERVER=%s;PORT=%d;COMMLOG=1;DEBUG=1;LOGDIR=logs/;RDSLOGTHRESHOLD=0;", dsn.c_str(), server.c_str(), port);
    }

    ConnectionStringBuilder(const std::string& str) { length += sprintf(conn_in, "%s", str.c_str()); }

    ConnectionStringBuilder(const std::wstring& wstr) {
        std::string str = StringHelper::ToString(wstr);
        length += sprintf(conn_in, "%s", str.c_str());
    }

    ConnectionStringBuilder& withUID(const std::string& uid) {
        length += sprintf(conn_in + length, "UID=%s;", uid.c_str());
        return *this;
    }

    ConnectionStringBuilder& withPWD(const std::string& pwd) {
        length += sprintf(conn_in + length, "PWD=%s;", pwd.c_str());
        return *this;
    }

    ConnectionStringBuilder& withDatabase(const std::string& db) {
        length += sprintf(conn_in + length, "DATABASE=%s;", db.c_str());
        return *this;
    }

    ConnectionStringBuilder& withFailoverMode(const std::string& failover_mode) {
        length += sprintf(conn_in + length, "FAILOVERMODE=%s;", failover_mode.c_str());
        return *this;
    }

    ConnectionStringBuilder& withEnableClusterFailover(const bool& enable_cluster_failover) {
        length += sprintf(conn_in + length, "ENABLECLUSTERFAILOVER=%d;", enable_cluster_failover ? 1 : 0);
        return *this;
    }

    ConnectionStringBuilder& withReaderHostSelectorStrategy(const std::string& strategy) {
        length += sprintf(conn_in + length, "READERHOSTSELECTORSTRATEGY=%s;", strategy.c_str());
        return *this;
    }

    ConnectionStringBuilder& withIgnoreTopologyRequest(const int& ignore_topology_request) {
        length += sprintf(conn_in + length, "IGNORETOPOLOGYREQUEST=%d;", ignore_topology_request);
        return *this;
    }

    ConnectionStringBuilder& withTopologyHighRefreshRate(const int& topology_high_refresh_rate) {
        length += sprintf(conn_in + length, "TOPOLOGYHIGHREFRESHRATE=%d;", topology_high_refresh_rate);
        return *this;
    }

    ConnectionStringBuilder& withTopologyRefreshRate(const int& topology_refresh_rate) {
        length += sprintf(conn_in + length, "TOPOLOGYREFRESHRATE=%d;", topology_refresh_rate);
        return *this;
    }
    ConnectionStringBuilder& withFailoverTimeout(const int& failover_t) {
        length += sprintf(conn_in + length, "FAILOVERTIMEOUT=%d;", failover_t);
        return *this;
    }

    ConnectionStringBuilder& withHostPattern(const std::string& host_pattern) {
        length += sprintf(conn_in + length, "HOSTPATTERN=%s;", host_pattern.c_str());
        return *this;
    }

    ConnectionStringBuilder& withAuthMode(const std::string& auth_mode) {
        length += sprintf(conn_in + length, "AUTHTYPE=%s;", auth_mode.c_str());
        return *this;
    }

    ConnectionStringBuilder& withAuthRegion(const std::string& auth_region) {
        length += sprintf(conn_in + length, "REGION=%s;", auth_region.c_str());
        return *this;
    }

    ConnectionStringBuilder& withAuthHost(const std::string& auth_host) {
        length += sprintf(conn_in + length, "IAMHOST=%s;", auth_host.c_str());
        return *this;
    }

    ConnectionStringBuilder& withAuthExpiration(const int& auth_expiration) {
        length += sprintf(conn_in + length, "TOKENEXPIRATION=%d;", auth_expiration);
        return *this;
    }

    ConnectionStringBuilder& withSecretId(const std::string& secret_id) {
        length += sprintf(conn_in + length, "SECRETID=%s;", secret_id.c_str());
        return *this;
    }

    ConnectionStringBuilder& withSslMode(const std::string& ssl_mode) {
        length += sprintf(conn_in + length, "SSLMODE=%s;", ssl_mode.c_str());
        return *this;
    }

    ConnectionStringBuilder& withLimitlessEnabled(const bool& limitless_enabled) {
        length += sprintf(conn_in + length, "LIMITLESSENABLED=%d;", limitless_enabled ? 1 : 0);
        return *this;
    }

    ConnectionStringBuilder& withLimitlessMode(const std::string& limitless_mode) {
        length += sprintf(conn_in + length, "LIMITLESSMODE=%s;", limitless_mode.c_str());
        return *this;
    }

    ConnectionStringBuilder& withLimitlessMonitorIntervalMs(const int& interval) {
        length += sprintf(conn_in + length, "LIMITLESSMONITORINTERVALMS=%d;", interval);
        return *this;
    }

    ConnectionStringBuilder& withLimitlessServiceId(const std::string& id) {
        length += sprintf(conn_in + length, "LIMITLESSSERVICEID=%s;", id.c_str());
        return *this;
    }

    #ifdef UNICODE
    std::wstring getString() const { return StringHelper::ToWstring(conn_in); }
    #else
    std::string getString() const { return conn_in; }
    #endif

   private:
    char conn_in[4096] = "\0";
    int length = 0;
};

#endif  // CONNECTION_STRING_BUILDER_H_
