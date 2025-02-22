// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2.0
// (GPLv2), as published by the Free Software Foundation, with the
// following additional permissions:
//
// This program is distributed with certain software that is licensed
// under separate terms, as designated in a particular file or component
// or in the license documentation. Without limiting your rights under
// the GPLv2, the authors of this program hereby grant you an additional
// permission to link the program and your derivative works with the
// separately licensed software that they have included with the program.
//
// Without limiting the foregoing grant of rights under the GPLv2 and
// additional permission as to separately licensed software, this
// program is also subject to the Universal FOSS Exception, version 1.0,
// a copy of which can be found along with its FAQ at
// http://oss.oracle.com/licenses/universal-foss-exception.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License, version 2.0, for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see
// http://www.gnu.org/licenses/gpl-2.0.html.

#ifndef CONNECTION_STRING_BUILDER_H_
#define CONNECTION_STRING_BUILDER_H_

#include <string>

class ConnectionStringBuilder {
   public:
    ConnectionStringBuilder(const std::string& dsn, const std::string& server, int port) {
        length += sprintf(conn_in, "DSN=%s;SERVER=%s;PORT=%d;", dsn.c_str(), server.c_str(), port);
    }

    ConnectionStringBuilder(const std::string& str) { length += sprintf(conn_in, "%s", str.c_str()); }

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

    ConnectionStringBuilder& withLogDir(const std::string& log_dir) {
        length += sprintf(conn_in + length, "LOGDIR=%s;", log_dir.c_str());
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

    std::string getString() const { return conn_in; }

   private:
    char conn_in[4096] = "\0";
    int length = 0;
};

#endif  // CONNECTION_STRING_BUILDER_H_
