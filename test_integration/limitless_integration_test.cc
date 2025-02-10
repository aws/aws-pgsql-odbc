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

#include <vector>

#include "connection_string_builder.h"
#include "integration_test_utils.h"

#define LIMITLESS_ROUTERS_QUERY "SELECT router_endpoint, load FROM aurora_limitless_router_endpoints()"

// Connection string parameters
static const char* test_dsn;
static const char* test_db;
static const char* test_user;
static const char* test_pwd;
static unsigned int test_port;

static std::string test_endpoint;
static std::string preferred_endpoint;

#include <iostream>

void get_preferred_endpoint() {
    test_endpoint = std::getenv("TEST_SERVER");
    test_dsn = std::getenv("TEST_DSN");
    test_db = std::getenv("TEST_DATABASE");
    test_user = std::getenv("TEST_USERNAME");
    test_pwd = std::getenv("TEST_PASSWORD");
    // Cast to remove const
    test_port = INTEGRATION_TEST_UTILS::str_to_int(
        INTEGRATION_TEST_UTILS::get_env_var("POSTGRES_PORT", (char*) "5432"));

    auto conn_str_builder = ConnectionStringBuilder();
    auto conn_str = conn_str_builder
        .withDSN(test_dsn)
        .withServer(test_endpoint)
        .withUID(test_user)
        .withPWD(test_pwd)
        .withPort(test_port)
        .withDatabase(test_db).build();

    SQLHENV env1 = nullptr;
    SQLHDBC dbc1 = nullptr;
    SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &env1);
    SQLSetEnvAttr(env1, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
    SQLAllocHandle(SQL_HANDLE_DBC, env1, &dbc1);

    SQLCHAR conn_out[4096] = "\0";
    SQLSMALLINT len;
    EXPECT_EQ(SQL_SUCCESS, 
        SQLDriverConnect(dbc1, nullptr, AS_SQLCHAR(conn_str.c_str()), SQL_NTS,
            conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT));

    SQLHSTMT stmt = nullptr;
    EXPECT_EQ(SQL_SUCCESS, SQLAllocHandle(SQL_HANDLE_STMT, dbc1, &stmt));
    EXPECT_EQ(SQL_SUCCESS, SQLExecDirect(stmt, AS_SQLCHAR(LIMITLESS_ROUTERS_QUERY), SQL_NTS));

    SQLCHAR router_endpoint_value[2049] = {0};
    SQLLEN ind_router_endpoint_value = 0;

    SQLCHAR load_value[5] = {0};
    SQLLEN ind_load_value = 0;

    EXPECT_EQ(SQL_SUCCESS, SQLBindCol(stmt, 1, SQL_C_CHAR, &router_endpoint_value, sizeof(router_endpoint_value), &ind_router_endpoint_value));
    EXPECT_EQ(SQL_SUCCESS, SQLBindCol(stmt, 2, SQL_C_CHAR, &load_value, sizeof(load_value), &ind_load_value));

    SQLLEN row_count = 0;
    EXPECT_EQ(SQL_SUCCESS, SQLRowCount(stmt, &row_count));
    EXPECT_TRUE(row_count > 0);

    SQLCHAR preferred_router[2049] = {0};
    int64_t greatest_weight = 0; // arbitrarily large value

    while (SQL_SUCCEEDED(SQLFetch(stmt))) {
        const double weight_scaling = 10;
        int64_t weight = std::round(weight_scaling - (atof(reinterpret_cast<const char *>(load_value)) * weight_scaling));
        if (weight > greatest_weight) {
            greatest_weight = weight;
            strncpy((char *)preferred_router, (char *)router_endpoint_value, 2049);
        }
    }

    preferred_endpoint = (const char *)preferred_router;

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

class LimitlessIntegrationTest : public testing::Test {
protected:
    ConnectionStringBuilder builder;
    SQLHENV env = nullptr;
    SQLHDBC dbc = nullptr;

    static void SetUpTestSuite() {
    }

    static void TearDownTestSuite() {
    }

    void SetUp() override {
        get_preferred_endpoint(); // query endpoint directly to get the newest preferred endpoint
        
        SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &env);
        SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
        SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

        builder = ConnectionStringBuilder();
        builder
            .withDSN(test_dsn)
            .withServer(test_endpoint)
            .withUID(test_user)
            .withPWD(test_pwd)
            .withPort(test_port)
            .withDatabase(test_db);
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
    auto connection_string = builder
        .withLimitlessEnabled(true)
        .withLimitlessMode("immediate")
        .withLimitlessMonitorIntervalMs(1000)
        .withLimitlessServiceId("test_id").build();

    SQLCHAR conn_out[4096] = "\0";
    SQLSMALLINT len;
    SQLRETURN rc = SQLDriverConnect(dbc, nullptr, AS_SQLCHAR(connection_string.c_str()), SQL_NTS,
        conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
    EXPECT_EQ(SQL_SUCCESS, rc);

    // connect server should be the preferred endpoint
    SQLCHAR server_name[256];
    rc = SQLGetInfo(dbc, SQL_SERVER_NAME, server_name, sizeof(server_name), &len);
    EXPECT_EQ(SQL_SUCCESS, rc);
    EXPECT_TRUE(std::string((const char *)server_name) == preferred_endpoint);

    rc = SQLDisconnect(dbc);
    EXPECT_EQ(SQL_SUCCESS, rc);
}

TEST_F(LimitlessIntegrationTest, MultipleConnections) {
    auto connection_string = builder
        .withLimitlessEnabled(true)
        .withLimitlessMode("immediate")
        .withLimitlessMonitorIntervalMs(30 * 1000) 
        .withLimitlessServiceId("test_id").build();

    std::vector<SQLHDBC> dbcs;

    // spin up ten connections to the db (increase weight on initially preferred endpoint)
    for (int i = 0; i < 10; i++) {
        SQLCHAR conn_out[4096] = "\0";
        SQLSMALLINT len;
        SQLHDBC _dbc;
        SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_DBC, env, &_dbc);
        EXPECT_EQ(SQL_SUCCESS, rc);
        if (rc != SQL_SUCCESS) return;
        rc = SQLDriverConnect(_dbc, nullptr, AS_SQLCHAR(connection_string.c_str()), SQL_NTS,
            conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
        EXPECT_EQ(SQL_SUCCESS, rc);
        if (rc != SQL_SUCCESS) return;

        dbcs.push_back(_dbc);
    }

    // sleep so that the monitor will get a new endpoint
    std::this_thread::sleep_for(std::chrono::milliseconds(30 * 1000));
    get_preferred_endpoint(); // query endpoint directly to get the newest preferred endpoint

    // open a new connection
    SQLCHAR conn_out[4096] = "\0";
    SQLSMALLINT len;
    SQLRETURN rc = SQLDriverConnect(dbc, nullptr, AS_SQLCHAR(connection_string.c_str()), SQL_NTS,
        conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
    EXPECT_EQ(SQL_SUCCESS, rc);
    if (rc != SQL_SUCCESS) return;

    // connect server should be the NEW preferred endpoint
    SQLCHAR server_name[256];
    rc = SQLGetInfo(dbc, SQL_SERVER_NAME, server_name, sizeof(server_name), &len);
    EXPECT_EQ(SQL_SUCCESS, rc);
    if (rc != SQL_SUCCESS) return;
    EXPECT_TRUE(std::string((const char *)server_name) == preferred_endpoint);

    rc = SQLDisconnect(dbc);

    // disconnect the 10 connections
    for (int i = 0; i < dbcs.size(); i++) {
        SQLHDBC _dbc = dbcs[i];
        rc = SQLDisconnect(_dbc);
    }
}
