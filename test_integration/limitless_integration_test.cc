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

// Connection string parameters
static const char* test_dsn;
static const char* test_db;
static const char* test_user;
static const char* test_pwd;
static unsigned int test_port;

static std::string test_endpoint;

class LimitlessIntegrationTest : public testing::Test {
protected:
    SQLHENV env = nullptr;
    SQLHDBC dbc = nullptr;

    static void SetUpTestSuite() {
        test_endpoint = std::getenv("TEST_SERVER");
        test_dsn = std::getenv("TEST_DSN");
        test_db = std::getenv("TEST_DATABASE");
        test_user = std::getenv("TEST_USERNAME");
        test_pwd = std::getenv("TEST_PASSWORD");
        // Cast to remove const
        test_port = INTEGRATION_TEST_UTILS::str_to_int(
            INTEGRATION_TEST_UTILS::get_env_var("POSTGRES_PORT", (char*) "5432"));
    }

    static void TearDownTestSuite() {
    }

    void SetUp() override {
        SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &env);
        SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
        SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
    }

    void TearDown() override {
        if (nullptr != dbc) {
            SQLFreeHandle(SQL_HANDLE_DBC, dbc);
        }
        if (nullptr != env) {
            SQLFreeHandle(SQL_HANDLE_ENV, env);
        }
    }
};

TEST_F(LimitlessIntegrationTest, SingleConnection) {
    auto connection_string = ConnectionStringBuilder(test_dsn, test_endpoint, test_port)
        .withUID(test_user)
        .withPWD(test_pwd)
        .withDatabase(test_db)
        .withLimitlessEnabled(true)
        .withLimitlessMode("immediate")
        .withLimitlessMonitorIntervalMs(1000)
        .withLimitlessServiceId("test_id").getString();

    SQLCHAR conn_out[4096] = "\0";
    SQLSMALLINT len;
    SQLRETURN rc = SQLDriverConnect(dbc, nullptr, AS_SQLCHAR(connection_string.c_str()), SQL_NTS,
        conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
    ASSERT_EQ(SQL_SUCCESS, rc);

    // server endpoint should no longer be the shard group endpoint
    SQLCHAR server_name[256];
    rc = SQLGetInfo(dbc, SQL_SERVER_NAME, server_name, sizeof(server_name), &len);
    EXPECT_EQ(SQL_SUCCESS, rc);
    EXPECT_TRUE(std::string((const char *)server_name) != test_endpoint);
    SQLDisconnect(dbc);
}

TEST_F(LimitlessIntegrationTest, MultipleConnections) {
    auto connection_string_1 = ConnectionStringBuilder(test_dsn, test_endpoint, test_port)
        .withUID(test_user)
        .withPWD(test_pwd)
        .withDatabase(test_db)
        .withLimitlessEnabled(false).getString();
    auto connection_string_2 = ConnectionStringBuilder(test_dsn, test_endpoint, test_port)
        .withUID(test_user)
        .withPWD(test_pwd)
        .withDatabase(test_db)
        .withLimitlessEnabled(true)
        .withLimitlessMode("immediate")
        .withLimitlessMonitorIntervalMs(1000) 
        .withLimitlessServiceId("test_id").getString();

    std::vector<SQLHDBC> dbcs;

    // spin up ten connections to the db
    for (int i = 0; i < 10; i++) {
        SQLCHAR conn_out[4096] = "\0";
        SQLSMALLINT len;
        SQLHDBC _dbc;
        SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_DBC, env, &_dbc);
        ASSERT_EQ(SQL_SUCCESS, rc);
        if (rc != SQL_SUCCESS) return;
        rc = SQLDriverConnect(_dbc, nullptr, AS_SQLCHAR(connection_string_1.c_str()), SQL_NTS,
            conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
        ASSERT_EQ(SQL_SUCCESS, rc);
        if (rc != SQL_SUCCESS) return;

        dbcs.push_back(_dbc);
    }

    // sleep so that the monitor will get a new endpoint
    std::this_thread::sleep_for(std::chrono::milliseconds(4 * 1000));

    // open a new connection
    SQLCHAR conn_out[4096] = "\0";
    SQLSMALLINT len;
    SQLRETURN rc = SQLDriverConnect(dbc, nullptr, AS_SQLCHAR(connection_string_2.c_str()), SQL_NTS,
        conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
    ASSERT_EQ(SQL_SUCCESS, rc);
    if (rc != SQL_SUCCESS) return;

    // server endpoint should no longer be the shard group endpoint
    SQLCHAR server_name[256];
    rc = SQLGetInfo(dbc, SQL_SERVER_NAME, server_name, sizeof(server_name), &len);
    EXPECT_EQ(SQL_SUCCESS, rc);
    if (rc != SQL_SUCCESS) return;
    EXPECT_TRUE(std::string((const char *)server_name) != test_endpoint);
    SQLDisconnect(dbc);

    // disconnect the 10 connections
    for (int i = 0; i < dbcs.size(); i++) {
        SQLHDBC _dbc = dbcs[i];
        SQLDisconnect(_dbc);
    }
}
