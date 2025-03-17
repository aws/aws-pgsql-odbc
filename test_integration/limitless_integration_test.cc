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

#include <iostream>
#include <thread>
#include <vector>

#include "connection_string_builder.h"
#include "integration_test_utils.h"

// from aws-rds-odbc
#include "round_robin_host_selector.h"
#include "limitless_monitor_service.h"
#include "limitless_query_helper.h"

#define MONITOR_INTERVAL_MS 15000
#define MONITOR_SERVICE_ID  "test_id"

#define NUM_CONNECTIONS_TO_OVERLOAD_ROUTER  20

// Connection string parameters
static const char* test_dsn;
static const char* test_db;
static const char* test_user;
static const char* test_pwd;
static unsigned int test_port;

static std::string shardgrp_endpoint;

SQLHENV env = nullptr;
SQLHDBC monitor_dbc = nullptr;

class RoundRobinHelper {
public:
    RoundRobinHelper() = default;
    ~RoundRobinHelper() = default;

    void UpdateHosts() {
        this->hosts = LimitlessQueryHelper::QueryForLimitlessRouters(monitor_dbc, test_port);
    }

    std::string GetRoundRobinHost() {
        // return round robin host on pre-existing host list
        std::unordered_map<std::string, std::string> properties;
        RoundRobinHostSelector::SetRoundRobinWeight(this->hosts, properties);
        HostInfo host = this->round_robin.GetHost(this->hosts, true, properties);

        return host.GetHost();
    }

    // all round robin instances have a shared cache; this clears it, so that expected hosts can be checked against
    void ClearCache() {
        this->round_robin.ClearCache();
    }

private:
    std::vector<HostInfo> hosts;
    RoundRobinHostSelector round_robin;
} round_robin_helper;

void load_thread(SQLTCHAR *conn_in) {
    SQLHDBC dbc;
    SQLHSTMT stmt;
    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
    if (rc != SQL_SUCCESS) {
        return;
    }

    rc = SQLDriverConnect(dbc, nullptr, conn_in, SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
    if (rc != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_DBC, dbc);
        return;
    }

    rc = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
    if (rc != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_DBC, dbc);
        return;
    }

    // run as many queries within two of the monitor intervals separated by 1s each time
    int nqueries = 2 * (MONITOR_INTERVAL_MS / 1000);

    for (int i = 0; i < nqueries; i++) {
        char query[] = "SELECT 1;";
        INTEGRATION_TEST_UTILS::exec_query(stmt, query);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    SQLDisconnect(dbc);
    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
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

        SQLRETURN rc = SQLDriverConnect(monitor_dbc, nullptr, AS_SQLTCHAR(monitor_connection_string.c_str()), SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
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

        // fresh slate per test
        round_robin_helper.ClearCache();
    }

    void TearDown() override {
        if (nullptr != dbc) {
            SQLFreeHandle(SQL_HANDLE_DBC, dbc);
            dbc = nullptr;
        }
    }
};

TEST_F(LimitlessIntegrationTest, ImmediateConnectionToRoundRobinHost) {
    // the service shouldn't be running right now
    ASSERT_FALSE(CheckLimitlessMonitorService(MONITOR_SERVICE_ID));

    // get the current expected host
    round_robin_helper.UpdateHosts();
    std::string expected_host = round_robin_helper.GetRoundRobinHost();
    round_robin_helper.ClearCache();

    auto connection_string = ConnectionStringBuilder(test_dsn, shardgrp_endpoint, test_port)
        .withUID(test_user)
        .withPWD(test_pwd)
        .withDatabase(test_db)
        .withLimitlessEnabled(true)
        .withLimitlessMode("immediate")
        .withLimitlessMonitorIntervalMs(1000)
        .withLimitlessServiceId(MONITOR_SERVICE_ID).getString();

    // start a connection
    SQLRETURN rc = SQLDriverConnect(dbc, nullptr, AS_SQLTCHAR(connection_string.c_str()), SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
    INTEGRATION_TEST_UTILS::print_errors(dbc, SQL_HANDLE_DBC);
    ASSERT_EQ(SQL_SUCCESS, rc);
    ASSERT_TRUE(CheckLimitlessMonitorService(MONITOR_SERVICE_ID));

    // connected server should be the expected host
    SQLTCHAR server_name[MAX_NAME_LEN] = {0};
    rc = SQLGetInfo(dbc, SQL_SERVER_NAME, server_name, sizeof(server_name), nullptr);
    ASSERT_EQ(StringHelper::ToString(server_name), expected_host);

    SQLDisconnect(dbc);
    // service should have stopped due to no live connections
    ASSERT_FALSE(CheckLimitlessMonitorService(MONITOR_SERVICE_ID));
}

TEST_F(LimitlessIntegrationTest, LazyConnectionToRoundRobinHost) {
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
    SQLTCHAR server_name[MAX_NAME_LEN] = {0};
    SQLRETURN rc;

    // initial connection should be via Route53, so the connected endpoint isn't important
    rc = SQLDriverConnect(dbc, nullptr, conn_in, SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
    INTEGRATION_TEST_UTILS::print_errors(dbc, SQL_HANDLE_DBC);
    ASSERT_EQ(SQL_SUCCESS, rc);
    ASSERT_TRUE(CheckLimitlessMonitorService(MONITOR_SERVICE_ID));

    // wait the full monitor interval to ensure the monitor service starts and gets new routers
    std::this_thread::sleep_for(std::chrono::milliseconds(MONITOR_INTERVAL_MS));

    // get the current expected host
    round_robin_helper.UpdateHosts();
    std::string expected_host = round_robin_helper.GetRoundRobinHost();
    round_robin_helper.ClearCache();

    // prepare a second connection
    SQLHDBC second_dbc;
    SQLAllocHandle(SQL_HANDLE_DBC, env, &second_dbc);
    rc = SQLDriverConnect(second_dbc, nullptr, conn_in, SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
    ASSERT_EQ(SQL_SUCCESS, rc);

    // should connect to the expected host
    rc = SQLGetInfo(second_dbc, SQL_SERVER_NAME, server_name, sizeof(server_name), nullptr);
    ASSERT_EQ(StringHelper::ToString(server_name), expected_host);

    SQLDisconnect(second_dbc);
    SQLFreeHandle(SQL_HANDLE_DBC, second_dbc);
    // service should stay online as there's another active connection
    ASSERT_TRUE(CheckLimitlessMonitorService(MONITOR_SERVICE_ID));

    SQLDisconnect(dbc);
    // service should now be stopped as the last connection has closed
    ASSERT_FALSE(CheckLimitlessMonitorService(MONITOR_SERVICE_ID));
}

TEST_F(LimitlessIntegrationTest, ConnectionSwitchDueToHighLoad) {
    // get the currently preferred host from a fresh round robin selection
    round_robin_helper.UpdateHosts();
    std::string initial_host = round_robin_helper.GetRoundRobinHost();
    round_robin_helper.ClearCache();

    auto load_conn_string = ConnectionStringBuilder(test_dsn, initial_host, test_port)
        .withUID(test_user)
        .withPWD(test_pwd)
        .withDatabase(test_db)
        .withLimitlessEnabled(false).getString();
    SQLTCHAR *conn_in = AS_SQLTCHAR(load_conn_string.c_str());

    std::vector<std::shared_ptr<std::thread>> load_threads;

    for (int i = 0; i < NUM_CONNECTIONS_TO_OVERLOAD_ROUTER; i++) {
        std::shared_ptr<std::thread> thread = std::make_shared<std::thread>(&load_thread, conn_in);
        load_threads.push_back(thread);
    }

    // service should not be running - the threads aren't using the limitless monitor service
    ASSERT_FALSE(CheckLimitlessMonitorService(MONITOR_SERVICE_ID));

    // wait the full interval such that the monitor's internal host information is updated
    std::this_thread::sleep_for(std::chrono::milliseconds(MONITOR_INTERVAL_MS));

    // collect all three of the expected hosts before the limitless monitor service interacts with the round robin cache
    // the limitless monitor service's calls to its round robin host should be the exact same, so these expected hosts should match what is connected to
    round_robin_helper.UpdateHosts();
    std::vector<std::string> expected_hosts;
    for (int i = 0; i < 3; i++) {
        expected_hosts.push_back(round_robin_helper.GetRoundRobinHost());
    }
    round_robin_helper.ClearCache();

    // round robin follows alphabetical order; despite current load conditions, the first connection should be the same as the initial host
    ASSERT_EQ(expected_hosts[0], initial_host);

    // create connection string for a limitless connection
    auto limitless_conn_string = ConnectionStringBuilder(test_dsn, shardgrp_endpoint, test_port)
        .withUID(test_user)
        .withPWD(test_pwd)
        .withDatabase(test_db)
        .withLimitlessEnabled(true)
        .withLimitlessMode("immediate")
        .withLimitlessMonitorIntervalMs(MONITOR_INTERVAL_MS)
        .withLimitlessServiceId("test_id").getString();
    conn_in = AS_SQLTCHAR(limitless_conn_string.c_str());

    std::vector<SQLHDBC> limitless_dbcs;

    // start up three limitless connections
    for (int i = 0; i < 3; i++) {
        SQLHDBC limitless_dbc;
        SQLAllocHandle(SQL_HANDLE_DBC, env, &limitless_dbc);
        limitless_dbcs.push_back(limitless_dbc);

        SQLRETURN rc = SQLDriverConnect(limitless_dbc, nullptr, conn_in, SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
        INTEGRATION_TEST_UTILS::print_errors(limitless_dbc, SQL_HANDLE_DBC);
        ASSERT_EQ(SQL_SUCCESS, rc);

        SQLTCHAR server_name[MAX_NAME_LEN] = {0};
        rc = SQLGetInfo(limitless_dbc, SQL_SERVER_NAME, server_name, sizeof(server_name), nullptr);

        // each connection via limitless should follow the round robin order
        ASSERT_EQ(StringHelper::ToString(server_name), expected_hosts[i]);
    }

    expected_hosts.clear();

    // cleanup limitless dbcs
    for (SQLHDBC limitless_dbc : limitless_dbcs) {
        SQLDisconnect(limitless_dbc);
        SQLFreeHandle(SQL_HANDLE_DBC, limitless_dbc);
    }
    limitless_dbcs.clear();

    // join all threads
    for (std::shared_ptr<std::thread> thread : load_threads) {
        thread->join();
    }
    load_threads.clear();

    // service should have stopped
    ASSERT_FALSE(CheckLimitlessMonitorService(MONITOR_SERVICE_ID));
}
