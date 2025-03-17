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

#include <gtest/gtest.h>

#ifdef WIN32
#include <windows.h>
#endif

#include <cmath>

#include <sql.h>
#include <sqlext.h>

#include <vector>

#include <iostream>

#include "connection_string_builder.h"
#include "integration_test_utils.h"

// from aws-rds-odbc
#include "round_robin_host_selector.h"
#include "limitless_monitor_service.h"
#include "limitless_query_helper.h"

#define MONITOR_INTERVAL_MS 15000
#define MONITOR_SERVICE_ID  "test_id"

#define MAX_CONNECTIONS_TO_OVERLOAD_ROUTER  20

// Connection string parameters
static const char* test_dsn;
static const char* test_db;
static const char* test_user;
static const char* test_pwd;
static unsigned int test_port;

static std::string shardgrp_endpoint;

static RoundRobinHostSelector round_robin;
static std::vector<HostInfo> hosts;

SQLHENV env = nullptr;
SQLHDBC monitor_dbc = nullptr;

void update_hosts() {
    // query for limitless routers + their load information
    hosts = LimitlessQueryHelper::QueryForLimitlessRouters(monitor_dbc, test_port);
}

std::string get_round_robin_host() {
    // return round robin host on pre-existing host list
    std::unordered_map<std::string, std::string> properties;
    RoundRobinHostSelector::SetRoundRobinWeight(hosts, properties);
    HostInfo host = round_robin.GetHost(hosts, true, properties);
    return host.GetHost();
}

class LimitlessIntegrationTest : public testing::Test {
protected:
    SQLHDBC dbc = nullptr;

    static void SetUpTestSuite() {
        shardgrp_endpoint = std::getenv("TEST_SERVER");
        test_dsn = std::getenv("TEST_DSN");
        test_db = std::getenv("TEST_DATABASE");
        test_user = std::getenv("TEST_USERNAME");
        test_pwd = std::getenv("TEST_PASSWORD");
        // Cast to remove const
        test_port = INTEGRATION_TEST_UTILS::str_to_int(
            INTEGRATION_TEST_UTILS::get_env_var("POSTGRES_PORT", (char*) "5432"));

        // allocate the env
        SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &env);
        SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
        SQLAllocHandle(SQL_HANDLE_DBC, env, &monitor_dbc);

        // connect the test monitor which will be used to fetch for limitless routers throughout the test suite
        auto monitor_connection_string = ConnectionStringBuilder(test_dsn, shardgrp_endpoint, test_port)
            .withUID(test_user)
            .withPWD(test_pwd)
            .withDatabase(test_db)
            .withLimitlessEnabled(false).getString();

        SQLTCHAR conn_out[4096] = {0};
        SQLSMALLINT len;
        SQLRETURN rc = SQLDriverConnect(monitor_dbc, nullptr, AS_SQLTCHAR(monitor_connection_string.c_str()), monitor_connection_string.size(),
            conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
        EXPECT_EQ(SQL_SUCCESS, rc);
        INTEGRATION_TEST_UTILS::print_errors(monitor_dbc, SQL_HANDLE_DBC);
    }

    static void TearDownTestSuite() {
        if (nullptr != monitor_dbc) {
            SQLDisconnect(monitor_dbc);
            SQLFreeHandle(SQL_HANDLE_DBC, monitor_dbc);
        }
        if (nullptr != env) {
            SQLFreeHandle(SQL_HANDLE_ENV, env);
        }
    }

    void SetUp() override {
        SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
    }

    void TearDown() override {
        if (nullptr != dbc) {
            SQLFreeHandle(SQL_HANDLE_DBC, dbc);
        }
    }
};

TEST_F(LimitlessIntegrationTest, ImmediateConnectionToLeastLoadedRouter) {
    // the service shouldn't be running right now
    ASSERT_FALSE(CheckLimitlessMonitorService(MONITOR_SERVICE_ID));

    auto connection_string = ConnectionStringBuilder(test_dsn, shardgrp_endpoint, test_port)
        .withUID(test_user)
        .withPWD(test_pwd)
        .withDatabase(test_db)
        .withLimitlessEnabled(true)
        .withLimitlessMode("immediate")
        .withLimitlessMonitorIntervalMs(1000)
        .withLimitlessServiceId(MONITOR_SERVICE_ID).getString();

    update_hosts();
    std::string expected_host = get_round_robin_host();

    // start a connection
    SQLTCHAR conn_out[4096] = {0};
    SQLSMALLINT len;
    SQLRETURN rc = SQLDriverConnect(dbc, nullptr, AS_SQLTCHAR(connection_string.c_str()), SQL_NTS,
        conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
    INTEGRATION_TEST_UTILS::print_errors(dbc, SQL_HANDLE_DBC);
    ASSERT_EQ(SQL_SUCCESS, rc);
    ASSERT_TRUE(CheckLimitlessMonitorService(MONITOR_SERVICE_ID));

    // connected server should be the expected host
    SQLTCHAR server_name[MAX_NAME_LEN] = {0};
    rc = SQLGetInfo(dbc, SQL_SERVER_NAME, server_name, sizeof(server_name), &len);
    ASSERT_EQ(StringHelper::ToString(server_name), expected_host);

    SQLDisconnect(dbc);
    // service should have stopped due to no live connections
    ASSERT_FALSE(CheckLimitlessMonitorService(MONITOR_SERVICE_ID));
}

TEST_F(LimitlessIntegrationTest, LazyConnectionToLeastLoadedRouter) {
    // the service shouldn't be running right now
    ASSERT_FALSE(CheckLimitlessMonitorService(MONITOR_SERVICE_ID));

    auto connection_string = ConnectionStringBuilder(test_dsn, shardgrp_endpoint, test_port)
        .withUID(test_user)
        .withPWD(test_pwd)
        .withDatabase(test_db)
        .withLimitlessEnabled(true)
        .withLimitlessMode("lazy")
        .withLimitlessMonitorIntervalMs(MONITOR_INTERVAL_MS)
        .withLimitlessServiceId("test_id").getString();

    SQLTCHAR *conn_in = AS_SQLTCHAR(connection_string.c_str());
    SQLTCHAR conn_out[MAX_NAME_LEN] = {0};
    SQLTCHAR server_name[MAX_NAME_LEN] = {0};
    SQLSMALLINT len;
    SQLRETURN rc;

    // initial connection should be via Route53, so the connected endpoint isn't important
    rc = SQLDriverConnect(dbc, nullptr, conn_in, SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
    INTEGRATION_TEST_UTILS::print_errors(dbc, SQL_HANDLE_DBC);
    ASSERT_EQ(SQL_SUCCESS, rc);
    ASSERT_TRUE(CheckLimitlessMonitorService(MONITOR_SERVICE_ID));

    // wait the full monitor interval to ensure the monitor service starts and gets new routers
    std::this_thread::sleep_for(std::chrono::milliseconds(MONITOR_INTERVAL_MS));

    // prepare a second connection
    update_hosts();
    std::string expected_host = get_round_robin_host();
    SQLHDBC second_dbc;
    SQLAllocHandle(SQL_HANDLE_DBC, env, &second_dbc);
    rc = SQLDriverConnect(second_dbc, nullptr, conn_in, SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
    ASSERT_EQ(SQL_SUCCESS, rc);

    // should connect to the expected host
    rc = SQLGetInfo(second_dbc, SQL_SERVER_NAME, server_name, sizeof(server_name), &len);
    ASSERT_EQ(StringHelper::ToString(server_name), expected_host);

    SQLDisconnect(second_dbc);
    // service should stay online as there's another active connection
    ASSERT_TRUE(CheckLimitlessMonitorService(MONITOR_SERVICE_ID));

    SQLDisconnect(dbc);
    // service should now be stopped as the last connection has closed
    ASSERT_FALSE(CheckLimitlessMonitorService(MONITOR_SERVICE_ID));
}

TEST_F(LimitlessIntegrationTest, ConnectionSwitchDueToLoadedRouter) {
    // the service shouldn't be running right now
    ASSERT_FALSE(CheckLimitlessMonitorService(MONITOR_SERVICE_ID));

    auto connection_string = ConnectionStringBuilder(test_dsn, shardgrp_endpoint, test_port)
        .withUID(test_user)
        .withPWD(test_pwd)
        .withDatabase(test_db)
        .withLimitlessEnabled(true)
        .withLimitlessMode("immediate")
        .withLimitlessMonitorIntervalMs(MONITOR_INTERVAL_MS)
        .withLimitlessServiceId("test_id").getString();

    SQLTCHAR *conn_in = AS_SQLTCHAR(connection_string.c_str());
    SQLTCHAR conn_out[MAX_NAME_LEN] = {0};
    SQLTCHAR server_name[MAX_NAME_LEN] = {0};
    SQLSMALLINT len;
    SQLRETURN rc;
    std::vector<SQLHDBC> load_dbcs;

    auto start_time = std::chrono::high_resolution_clock::now();

    // update host list as expected within limitless
    update_hosts();

    // start up as many connections within the interval as possible, or MAX_CONNECTIONS_TO_OVERLOAD_ROUTER, whichever comes first
    for (int i = 0; i < MAX_CONNECTIONS_TO_OVERLOAD_ROUTER; i++) {
        // allocate connection handle for load connection
        SQLHDBC load_dbc;
        SQLAllocHandle(SQL_HANDLE_DBC, env, &load_dbc);

        // open connection
        rc = SQLDriverConnect(load_dbc, nullptr, conn_in, SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
        INTEGRATION_TEST_UTILS::print_errors(load_dbc, SQL_HANDLE_DBC);
        ASSERT_EQ(SQL_SUCCESS, rc);

        // ensure it's connected to the current round robin host
        rc = SQLGetInfo(load_dbc, SQL_SERVER_NAME, server_name, sizeof(server_name), &len);
        std::string round_robin_host = get_round_robin_host();
        ASSERT_EQ(StringHelper::ToString(server_name), round_robin_host);

        load_dbcs.push_back(load_dbc);

        // check time elapsed and break if half the interval has passed
        auto now_time = std::chrono::high_resolution_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(now_time - start_time);
        if (elapsed_time.count() > MONITOR_INTERVAL_MS) {
            break;
        }
    }

    // service should be running
    ASSERT_TRUE(CheckLimitlessMonitorService(MONITOR_SERVICE_ID));

    // wait the remaining time if MAX_CONNECTIONS_TO_OVERLOAD_ROUTER was met, or wait a second (elapsed_time.count() would be >= MONITOR_INTERVAL_MS)
    auto now_time = std::chrono::high_resolution_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(now_time - start_time);
    auto remaining_time = MONITOR_INTERVAL_MS - elapsed_time.count();
    std::this_thread::sleep_for(std::chrono::milliseconds(remaining_time < 1000 ? 1000 : remaining_time));

    // update host list as limitless monitor should've updated its list, and get the newest round robin host
    update_hosts();
    std::string expected_host = get_round_robin_host();

    // start up a new connection on the same monitor
    rc = SQLDriverConnect(dbc, nullptr, conn_in, SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
    INTEGRATION_TEST_UTILS::print_errors(dbc, SQL_HANDLE_DBC);
    ASSERT_EQ(SQL_SUCCESS, rc);

    // ensure it's connected to the expected host
    rc = SQLGetInfo(dbc, SQL_SERVER_NAME, server_name, sizeof(server_name), &len);
    ASSERT_EQ(StringHelper::ToString(server_name), expected_host);

    // cleanup
    SQLDisconnect(dbc);
    for (SQLHDBC load_dbc : load_dbcs) {
        SQLDisconnect(load_dbc);
    }
    // service should have stopped
    ASSERT_FALSE(CheckLimitlessMonitorService(MONITOR_SERVICE_ID));
}
