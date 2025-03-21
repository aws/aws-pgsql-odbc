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

#include <cassert>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <iostream>
#include <random>
#include <stdexcept>

#include "connection_string_builder.h"
#include "integration_test_utils.h"

static std::string auth_type = "secrets-manager";

class SecretsManagerIntegrationTest : public testing::Test {
   protected:
    std::string SECRETS_ARN = std::getenv("SECRETS_ARN");
    char* dsn = std::getenv("TEST_DSN");
    char* test_db = std::getenv("TEST_DATABASE");

    int PG_PORT = INTEGRATION_TEST_UTILS::str_to_int(
        INTEGRATION_TEST_UTILS::get_env_var("POSTGRES_PORT", (char*) "5432"));
    char* TEST_REGION = INTEGRATION_TEST_UTILS::get_env_var("RDS_REGION", (char*) "us-east-2");
    std::string DB_SERVER_URL = std::getenv("TEST_SERVER");

    SQLHENV env = nullptr;
    SQLHDBC dbc = nullptr;

    #ifdef UNICODE
    std::wstring connection_string = L"";
    #else
    std::string connection_string = "";
    #endif

    static void SetUpTestSuite() {
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

TEST_F(SecretsManagerIntegrationTest, EnableSecretsManagerWithRegion) {
    connection_string = ConnectionStringBuilder(dsn, DB_SERVER_URL, PG_PORT)
                            .withDatabase(test_db)
                            .withAuthMode(auth_type)
                            .withAuthRegion(TEST_REGION)
                            .withSecretId(SECRETS_ARN)
                            .getString();
    SQLTCHAR conn_out[4096] = {0};
    SQLSMALLINT len;
    SQLRETURN rc = SQLDriverConnect(dbc, nullptr, AS_SQLTCHAR(connection_string.c_str()), SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
    EXPECT_EQ(SQL_SUCCESS, rc);
    EXPECT_EQ(SQL_SUCCESS, SQLDisconnect(dbc));
}

TEST_F(SecretsManagerIntegrationTest, EnableSecretsManagerWithoutRegion) {
    connection_string = ConnectionStringBuilder(dsn, DB_SERVER_URL, PG_PORT)
                            .withDatabase(test_db)
                            .withAuthMode(auth_type)
                            .withSecretId(SECRETS_ARN)
                            .getString();

    SQLTCHAR conn_out[4096] = {0};
    SQLSMALLINT len;
    SQLRETURN rc = SQLDriverConnect(dbc, nullptr, AS_SQLTCHAR(connection_string.c_str()), SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
    
    EXPECT_EQ(SQL_SUCCESS, rc);
    EXPECT_EQ(SQL_SUCCESS, SQLDisconnect(dbc));
}

// Passing in a wrong region should still work in retrieving secrets
// A full secret ARN will contain the proper region
TEST_F(SecretsManagerIntegrationTest, EnableSecretsManagerWrongRegion) {
    connection_string = ConnectionStringBuilder(dsn, DB_SERVER_URL, PG_PORT)
                            .withDatabase(test_db)
                            .withAuthMode(auth_type)
                            .withAuthRegion("us-fake-1")
                            .withSecretId(SECRETS_ARN)
                            .getString();

    SQLTCHAR conn_out[4096] = {0};
    SQLSMALLINT len;
    SQLRETURN rc = SQLDriverConnect(dbc, nullptr, AS_SQLTCHAR(connection_string.c_str()), SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
    EXPECT_EQ(SQL_SUCCESS, rc);
    EXPECT_EQ(SQL_SUCCESS, SQLDisconnect(dbc));
}

TEST_F(SecretsManagerIntegrationTest, EnableSecretsManagerInvalidSecretID) {
    connection_string = ConnectionStringBuilder(dsn, DB_SERVER_URL, PG_PORT)
                            .withDatabase(test_db)
                            .withAuthMode(auth_type)
                            .withAuthRegion(TEST_REGION)
                            .withSecretId("invalid-id")
                            .getString();
    SQLTCHAR conn_out[4096] = {0};
    SQLSMALLINT len;
    SQLRETURN rc = SQLDriverConnect(dbc, nullptr, AS_SQLTCHAR(connection_string.c_str()), SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
    EXPECT_EQ(SQL_ERROR, rc);

    // Check state
    SQLTCHAR sqlstate[6] = {0}, message[SQL_MAX_MESSAGE_LENGTH] = {0};
    SQLINTEGER native_error = 0;
    SQLSMALLINT stmt_length;
    EXPECT_EQ(SQL_SUCCESS, SQLError(nullptr, dbc, nullptr, sqlstate, &native_error, message, SQL_MAX_MESSAGE_LENGTH - 1, &stmt_length));
    #ifdef UNICODE
    EXPECT_EQ(L"08S01", std::wstring(AS_WCHAR(sqlstate)));
    #else
    EXPECT_EQ("08S01", std::string(AS_CHAR(sqlstate)));
    #endif
}
