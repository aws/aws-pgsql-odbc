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
static std::wstring test_endpointw;

class LimitlessIntegrationTest : public testing::Test {
protected:
    SQLHENV env = nullptr;
    SQLHDBC dbc = nullptr;

    static void SetUpTestSuite() {
        test_endpoint = std::getenv("TEST_SERVER");
        test_endpointw = StringHelper::ToWstring(test_endpoint);
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

    SQLTCHAR conn_out[4096] = {0};
    SQLSMALLINT len;
    SQLRETURN rc = SQLDriverConnect(dbc, nullptr, AS_SQLTCHAR(connection_string.c_str()), SQL_NTS,
        conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
    EXPECT_EQ(SQL_SUCCESS, rc);
    INTEGRATION_TEST_UTILS::print_errors(dbc, SQL_HANDLE_DBC);

    // server endpoint should no longer be the shard group endpoint
    SQLTCHAR server_name[256] = {0};
    rc = SQLGetInfo(dbc, SQL_SERVER_NAME, server_name, sizeof(server_name), &len);
    EXPECT_EQ(SQL_SUCCESS, rc);
    #ifdef UNICODE
    EXPECT_TRUE(std::wstring(AS_WCHAR(server_name)) != test_endpointw);
    #else
    EXPECT_TRUE(std::string(AS_CHAR(server_name)) != test_endpoint);
    #endif
    SQLDisconnect(dbc);
}
