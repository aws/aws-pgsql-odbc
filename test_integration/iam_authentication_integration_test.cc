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

#include <sql.h>
#include <sqlext.h>

#include <cstring> // memset

#include "connection_string_builder.h"
#include "integration_test_utils.h"

// Connection string parameters
static char* test_dsn;
static char* test_db;
static char* test_user;
static char* test_pwd;
static unsigned int test_port;
static char* iam_user;
static char* test_region;

static std::string test_endpoint;
#ifdef UNICODE
static std::wstring default_connection_string;
#else
static std::string default_connection_string;
#endif

#include <iostream>

class IamAuthenticationIntegrationTest : public testing::Test {
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
        test_port = INTEGRATION_TEST_UTILS::str_to_int(INTEGRATION_TEST_UTILS::get_env_var("POSTGRES_PORT", (char*)"5432"));
        iam_user = INTEGRATION_TEST_UTILS::get_env_var("IAM_USER", (char*)"john_doe");
        test_region = INTEGRATION_TEST_UTILS::get_env_var("RDS_REGION", (char*)"us-east-2");

        // Connect to execute SQL query to add IAM user to DB
        auto conn_str =
            ConnectionStringBuilder(test_dsn, test_endpoint, test_port).withUID(test_user).withPWD(test_pwd).withDatabase(test_db).getString();

        SQLHENV env1 = nullptr;
        SQLHDBC dbc1 = nullptr;
        SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &env1);
        SQLSetEnvAttr(env1, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
        SQLAllocHandle(SQL_HANDLE_DBC, env1, &dbc1);
        #ifdef UNICODE
        std::cout << StringHelper::ToString(conn_str) << std::endl;
        #else
        std::cout << conn_str << std::endl;
        #endif

        SQLTCHAR conn_out[4096] = {0};
        SQLSMALLINT len;
        SQLRETURN rc = SQLDriverConnect(dbc1, nullptr, AS_SQLTCHAR(conn_str.c_str()), SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
        EXPECT_EQ(SQL_SUCCESS, rc);

        SQLSMALLINT sl;
        SQLINTEGER er;

        SQLTCHAR sqlstate[6], message[4096];
        SQLRETURN err_rc = SQLError(nullptr,
                                    dbc1,
                                    nullptr,
                                    sqlstate,
                                    &er,
                                    message,
                                    SQL_MAX_MESSAGE_LENGTH - 1,
                                    &sl);

        if (SQL_SUCCEEDED(err_rc)) {
            #ifdef UNICODE
            std::cout << StringHelper::ToString(sqlstate) << ": " << StringHelper::ToString(message) << std::endl;
            #else
            std::cout << sqlstate << ": " << message << std::endl;
            #endif
        }
        SQLHSTMT stmt = nullptr;
        EXPECT_EQ(SQL_SUCCESS, SQLAllocHandle(SQL_HANDLE_STMT, dbc1, &stmt));

        char query_buffer[200];
        sprintf(query_buffer, "DROP USER IF EXISTS %s;", iam_user);
        #ifdef UNICODE
        std::wstring wquery_buffer = StringHelper::ToWstring(query_buffer);
        SQLExecDirect(stmt, AS_SQLTCHAR(wquery_buffer.c_str()), SQL_NTS);
        #else
        SQLExecDirect(stmt, AS_SQLTCHAR(query_buffer), SQL_NTS);
        #endif
        INTEGRATION_TEST_UTILS::print_errors(stmt, SQL_HANDLE_STMT);

        memset(query_buffer, 0, sizeof(query_buffer));
        sprintf(query_buffer, "CREATE USER %s;", iam_user);
        #ifdef UNICODE
        wquery_buffer = StringHelper::ToWstring(query_buffer);
        EXPECT_EQ(SQL_SUCCESS, SQLExecDirect(stmt, AS_SQLTCHAR(wquery_buffer.c_str()), SQL_NTS));
        #else
        EXPECT_EQ(SQL_SUCCESS, SQLExecDirect(stmt, AS_SQLTCHAR(query_buffer), SQL_NTS));
        #endif
        INTEGRATION_TEST_UTILS::print_errors(stmt, SQL_HANDLE_STMT);

        memset(query_buffer, 0, sizeof(query_buffer));
        sprintf(query_buffer, "GRANT rds_iam TO %s;", iam_user);
        #ifdef UNICODE
        wquery_buffer = StringHelper::ToWstring(query_buffer);
        EXPECT_EQ(SQL_SUCCESS, SQLExecDirect(stmt, AS_SQLTCHAR(wquery_buffer.c_str()), SQL_NTS));
        #else
        EXPECT_EQ(SQL_SUCCESS, SQLExecDirect(stmt, AS_SQLTCHAR(query_buffer), SQL_NTS));
        #endif
        INTEGRATION_TEST_UTILS::print_errors(stmt, SQL_HANDLE_STMT);

        EXPECT_EQ(SQL_SUCCESS, SQLFreeHandle(SQL_HANDLE_STMT, stmt));
        EXPECT_EQ(SQL_SUCCESS, SQLDisconnect(dbc1));

        if (nullptr != stmt) {
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        }
        if (nullptr != dbc1) {
            SQLDisconnect(dbc1);
            SQLFreeHandle(SQL_HANDLE_DBC, dbc1);
        }
        if (nullptr != env1) {
            SQLFreeHandle(SQL_HANDLE_ENV, env1);
        }
    }

    static void TearDownTestSuite() {}

    void SetUp() override {
        SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &env);
        SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
        SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

        default_connection_string = ConnectionStringBuilder(test_dsn, test_endpoint, test_port)
                                        .withUID(iam_user)
                                        .withDatabase(test_db)
                                        .withAuthMode("IAM")
                                        .withAuthRegion(test_region)
                                        .withAuthExpiration(900)
                                        .withSslMode("allow")
                                        .getString();
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

// Tests a simple IAM connection with all expected fields provided.
TEST_F(IamAuthenticationIntegrationTest, SimpleIamConnection) {
    SQLTCHAR conn_out[4096] = {0};
    SQLSMALLINT len;
    SQLRETURN rc = SQLDriverConnect(dbc, nullptr, AS_SQLTCHAR(default_connection_string.c_str()), SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
    EXPECT_EQ(SQL_SUCCESS, rc);

    rc = SQLDisconnect(dbc);
    EXPECT_EQ(SQL_SUCCESS, rc);
}

// Tests that IAM connection will still connect
// when given an IP address instead of a cluster name.
TEST_F(IamAuthenticationIntegrationTest, ConnectToIpAddress) {
    auto ip_address = INTEGRATION_TEST_UTILS::host_to_IP(test_endpoint);

    auto connection_string = ConnectionStringBuilder(test_dsn, ip_address, test_port)
                                 .withUID(iam_user)
                                 .withDatabase(test_db)
                                 .withAuthMode("IAM")
                                 .withAuthHost(test_endpoint)
                                 .withAuthRegion(test_region)
                                 .withAuthExpiration(900)
                                 .withSslMode("allow")
                                 .getString();

    SQLTCHAR conn_out[4096] = {0};
    SQLSMALLINT len;
    SQLRETURN rc = SQLDriverConnect(dbc, nullptr, AS_SQLTCHAR(connection_string.c_str()), SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
    EXPECT_EQ(SQL_SUCCESS, rc);

    rc = SQLDisconnect(dbc);
    EXPECT_EQ(SQL_SUCCESS, rc);
}

// Tests that IAM connection will still connect
// when given a wrong password (because the password gets replaced by the auth token).
TEST_F(IamAuthenticationIntegrationTest, WrongPassword) {
    auto connection_string = ConnectionStringBuilder(default_connection_string).withPWD("WRONG_PASSWORD").getString();

    SQLTCHAR conn_out[4096] = {0};
    SQLSMALLINT len;
    SQLRETURN rc = SQLDriverConnect(dbc, nullptr, AS_SQLTCHAR(connection_string.c_str()), SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
    EXPECT_EQ(SQL_SUCCESS, rc);

    rc = SQLDisconnect(dbc);
    EXPECT_EQ(SQL_SUCCESS, rc);
}

// Tests that the IAM connection will fail when provided a wrong user.
TEST_F(IamAuthenticationIntegrationTest, WrongUser) {
    auto connection_string = ConnectionStringBuilder(test_dsn, test_endpoint, test_port)
                                 .withUID("WRONG_USER")
                                 .withDatabase(test_db)
                                 .withAuthMode("IAM")
                                 .withAuthRegion(test_region)
                                 .withAuthExpiration(900)
                                 .withSslMode("allow")
                                 .getString();

    SQLTCHAR conn_out[4096] = {0};
    SQLSMALLINT len;
    SQLRETURN rc = SQLDriverConnect(dbc, nullptr, AS_SQLTCHAR(connection_string.c_str()), SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
    EXPECT_EQ(SQL_ERROR, rc);

    SQLSMALLINT stmt_length;
    SQLINTEGER native_err;
    SQLTCHAR msg[SQL_MAX_MESSAGE_LENGTH] = {0}, state[6] = {0};
    rc = SQLError(nullptr, dbc, nullptr, state, &native_err, msg, SQL_MAX_MESSAGE_LENGTH - 1, &stmt_length);
    EXPECT_EQ(SQL_SUCCESS, rc);
    #ifdef UNICODE
    EXPECT_EQ(L"08001", std::wstring(AS_WCHAR(state)));
    #else
    EXPECT_EQ("08001", std::string(AS_CHAR(state)));
    #endif
}

// Tests that the IAM connection will fail when provided an empty user.
TEST_F(IamAuthenticationIntegrationTest, EmptyUser) {
    auto connection_string = ConnectionStringBuilder(test_dsn, test_endpoint, test_port)
                                 .withUID("")
                                 .withDatabase(test_db)
                                 .withAuthMode("IAM")
                                 .withAuthRegion(test_region)
                                 .withAuthExpiration(900)
                                 .withSslMode("allow")
                                 .getString();

    SQLTCHAR conn_out[4096] = {0};
    SQLSMALLINT len;
    SQLRETURN rc = SQLDriverConnect(dbc, nullptr, AS_SQLTCHAR(connection_string.c_str()), SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
    EXPECT_EQ(SQL_ERROR, rc);

    SQLSMALLINT stmt_length;
    SQLINTEGER native_err;
    SQLTCHAR msg[SQL_MAX_MESSAGE_LENGTH] = {0}, state[6] = {0};
    rc = SQLError(nullptr, dbc, nullptr, state, &native_err, msg, SQL_MAX_MESSAGE_LENGTH - 1, &stmt_length);
    EXPECT_EQ(SQL_SUCCESS, rc);
    #ifdef UNICODE
    EXPECT_EQ(L"08001", std::wstring(AS_WCHAR(state)));
    #else
    EXPECT_EQ("08001", std::string(AS_CHAR(state)));
    #endif
}
