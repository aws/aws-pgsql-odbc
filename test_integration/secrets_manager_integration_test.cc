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

class SecretsManagerIntegrationTest : public testing::Test {
   protected:
    std::string SECRETS_ARN = std::getenv("SECRETS_ARN");
    char* dsn = std::getenv("TEST_DSN");

    int PG_PORT = INTEGRATION_TEST_UTILS::str_to_int(
        INTEGRATION_TEST_UTILS::get_env_var("POSTGRES_PORT", (char*) "5432"));
    char* TEST_REGION = INTEGRATION_TEST_UTILS::get_env_var("RDS_REGION", (char*) "us-east-2");
    std::string DB_SERVER_URL = std::getenv("TEST_SERVER");

    SQLHENV env = nullptr;
    SQLHDBC dbc = nullptr;

    ConnectionStringBuilder builder;
    std::string connection_string;

    static void SetUpTestSuite() {
    }

    static void TearDownTestSuite() {
    }

    void SetUp() override {
        SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &env);
        SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
        SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

        builder = ConnectionStringBuilder();
        builder.withPort(PG_PORT)
            .withAuthMode("secrets-manager");
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
    connection_string = builder
                            .withDSN(dsn)
                            .withServer(DB_SERVER_URL)
                            .withAuthRegion(TEST_REGION)
                            .withSecretId(SECRETS_ARN)
                            .build();
    SQLCHAR conn_out[4096] = "\0";
    SQLSMALLINT len;

    EXPECT_EQ(SQL_SUCCESS, SQLDriverConnect(dbc, nullptr, AS_SQLCHAR(connection_string.c_str()), SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT));
    EXPECT_EQ(SQL_SUCCESS, SQLDisconnect(dbc));
}

// Disabled until bug fix. Driver will always default to us-east-1
// and does not try to parse the ARN
TEST_F(SecretsManagerIntegrationTest, EnableSecretsManagerWithoutRegion) {
    connection_string = builder
                            .withDSN(dsn)
                            .withServer(DB_SERVER_URL)
                            .withSecretId(SECRETS_ARN)
                            .build();
    SQLCHAR conn_out[4096] = "\0";
    SQLSMALLINT len;

    EXPECT_EQ(SQL_SUCCESS, SQLDriverConnect(dbc, nullptr, AS_SQLCHAR(connection_string.c_str()), SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT));
    EXPECT_EQ(SQL_SUCCESS, SQLDisconnect(dbc));
}

// Disabled until bug fix. On failure, driver does not supply errors
TEST_F(SecretsManagerIntegrationTest, EnableSecretsManagerWrongRegion) {
    connection_string = builder
                            .withDSN(dsn)
                            .withServer(DB_SERVER_URL)
                            .withAuthRegion("us-east-1")
                            .withSecretId(SECRETS_ARN)
                            .build();
    SQLCHAR conn_out[4096] = "\0";
    SQLSMALLINT len;

    EXPECT_EQ(SQL_ERROR, SQLDriverConnect(dbc, nullptr, AS_SQLCHAR(connection_string.c_str()), SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT));

    // Check state
    SQLCHAR sqlstate[6] = "\0", message[SQL_MAX_MESSAGE_LENGTH] = "\0";;
    SQLINTEGER native_error = 0;
    SQLSMALLINT stmt_length;
    EXPECT_EQ(SQL_SUCCESS, SQLError(nullptr, dbc, nullptr, sqlstate, &native_error, message, SQL_MAX_MESSAGE_LENGTH - 1, &stmt_length));
    const std::string state = reinterpret_cast<char*>(sqlstate);
    EXPECT_EQ("08S01", state);
}

// Disabled until bug fix. On failure, driver does not supply errors
TEST_F(SecretsManagerIntegrationTest, EnableSecretsManagerInvalidSecretID) {
    connection_string = builder
                            .withDSN(dsn)
                            .withServer(DB_SERVER_URL)
                            .withAuthRegion(TEST_REGION)
                            .withSecretId("invalid-id")
                            .build();
    SQLCHAR conn_out[4096] = "\0";
    SQLSMALLINT len;

    EXPECT_EQ(SQL_ERROR, SQLDriverConnect(dbc, nullptr, AS_SQLCHAR(connection_string.c_str()), SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT));

    // Check state
    SQLCHAR sqlstate[6] = "\0", message[SQL_MAX_MESSAGE_LENGTH] = "\0";;
    SQLINTEGER native_error = 0;
    SQLSMALLINT stmt_length;
    EXPECT_EQ(SQL_SUCCESS, SQLError(nullptr, dbc, nullptr, sqlstate, &native_error, message, SQL_MAX_MESSAGE_LENGTH - 1, &stmt_length));
    const std::string state = reinterpret_cast<char*>(sqlstate);
    EXPECT_EQ("08S01", state);
}
