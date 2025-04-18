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

#include "base_failover_integration_test.cc"

#include "connection_string_builder.h"

class FailoverIntegrationTest : public BaseFailoverIntegrationTest {
   protected:
    std::string ACCESS_KEY = std::getenv("AWS_ACCESS_KEY_ID");
    std::string SECRET_ACCESS_KEY = std::getenv("AWS_SECRET_ACCESS_KEY");
    std::string SESSION_TOKEN = std::getenv("AWS_SESSION_TOKEN") ? std::getenv("AWS_SESSION_TOKEN") : "";
    std::string RDS_ENDPOINT = std::getenv("RDS_ENDPOINT") ? std::getenv("RDS_ENDPOINT") : "";
    std::string RDS_REGION = std::getenv("RDS_REGION") ? std::getenv("RDS_REGION") : "";
    Aws::RDS::RDSClientConfiguration client_config;
    Aws::RDS::RDSClient rds_client;
    SQLHENV env = nullptr;
    SQLHDBC dbc = nullptr;

    static void SetUpTestSuite() { Aws::InitAPI(options); }

    static void TearDownTestSuite() { Aws::ShutdownAPI(options); }

    void SetUp() override {
        SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &env);
        SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
        SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

        Aws::Auth::AWSCredentials credentials =
            SESSION_TOKEN.empty() ? Aws::Auth::AWSCredentials(Aws::String(ACCESS_KEY), Aws::String(SECRET_ACCESS_KEY))
                                  : Aws::Auth::AWSCredentials(Aws::String(ACCESS_KEY), Aws::String(SECRET_ACCESS_KEY), Aws::String(SESSION_TOKEN));
        client_config.region = RDS_REGION.empty() ? "us-east-2" : RDS_REGION;
        if (!RDS_ENDPOINT.empty()) {
            client_config.endpointOverride = RDS_ENDPOINT;
        }
        rds_client = Aws::RDS::RDSClient(credentials, client_config);

        cluster_instances = retrieve_topology_via_SDK(rds_client, cluster_id);
        writer_id = get_writer_id(cluster_instances);
        writer_endpoint = get_endpoint(writer_id);
        readers = get_readers(cluster_instances);
        reader_id = get_first_reader_id(cluster_instances);
        target_writer_id = get_random_DB_cluster_reader_instance_id(readers);

        default_connection_string = ConnectionStringBuilder(test_dsn, writer_endpoint, test_port)
                                        .withUID(test_user)
                                        .withPWD(test_pwd)
                                        .withDatabase(test_db)
                                        .withEnableClusterFailover(true)
                                        .getString();
        // Simple check to see if cluster is available.
        wait_until_cluster_has_right_state(rds_client, cluster_id);
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

/** Writer fails to a reader **/
TEST_F(FailoverIntegrationTest, WriterFailToReadear) {
	SQLTCHAR* conn_out;
    SQLSMALLINT len;

    auto conn_str = ConnectionStringBuilder(default_connection_string).withFailoverMode("STRICT_READER").getString(); 
    EXPECT_EQ(SQL_SUCCESS, SQLDriverConnect(dbc, nullptr, AS_SQLTCHAR(conn_str.c_str()), SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT));

    // Query new ID after failover
    std::string current_connection_id = query_instance_id(dbc);

    // Check if current connection is a reader
    EXPECT_TRUE(is_DB_instance_writer(rds_client, cluster_id, current_connection_id));

    failover_cluster_and_wait_until_writer_changed(rds_client, cluster_id, writer_id, target_writer_id);
    assert_query_failed(dbc, SERVER_ID_QUERY, ERROR_FAILOVER_SUCCEEDED);

    // Query new ID after failover
    current_connection_id = query_instance_id(dbc);

    // Check if current connection is a reader
    EXPECT_FALSE(is_DB_instance_writer(rds_client, cluster_id, current_connection_id));

    // Clean up test
    EXPECT_EQ(SQL_SUCCESS, SQLDisconnect(dbc));
}

/** Writer fails within a transaction. Open transaction by explicitly calling BEGIN */
TEST_F(FailoverIntegrationTest, WriterFailWithinTransaction_DisableAutocommit) {
    SQLSMALLINT len;
	EXPECT_EQ(SQL_SUCCESS,
			  SQLDriverConnect(dbc, nullptr, AS_SQLTCHAR(default_connection_string.c_str()), SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT));

    // Setup tests
    SQLHSTMT handle;
    EXPECT_EQ(SQL_SUCCESS, SQLAllocHandle(SQL_HANDLE_STMT, dbc, &handle));
    auto drop_table_query = AS_SQLTCHAR("DROP TABLE IF EXISTS failover_transaction_1");
    auto create_table_query = AS_SQLTCHAR("CREATE TABLE failover_transaction_1 (id INT NOT NULL PRIMARY KEY, failover_transaction_1_field VARCHAR(255) NOT NULL)");

    // Execute setup query
    EXPECT_TRUE(SQL_SUCCEEDED(SQLExecDirect(handle, drop_table_query, SQL_NTS)));
    EXPECT_EQ(SQL_SUCCESS, SQLExecDirect(handle, create_table_query, SQL_NTS));

    // Execute queries within the transaction
    auto insert_query_a = AS_SQLTCHAR("BEGIN; INSERT INTO failover_transaction_1 VALUES (1, 'test field string 1')");
    EXPECT_EQ(SQL_SUCCESS, SQLExecDirect(handle, insert_query_a, SQL_NTS));

    failover_cluster_and_wait_until_writer_changed(rds_client, cluster_id, writer_id, target_writer_id);

    // If there is an active transaction, roll it back and return an error 08007.
    assert_query_failed(dbc, SERVER_ID_QUERY, ERROR_TRANSACTION_UNKNOWN);
    // Query new ID after failover
    std::string current_connection_id = query_instance_id(dbc);

    // Check if current connection is a new writer
    EXPECT_TRUE(is_DB_instance_writer(rds_client, cluster_id, current_connection_id));
    EXPECT_NE(current_connection_id, writer_id);

    // No rows should have been inserted to the table
    EXPECT_EQ(0, query_count_table_rows(handle, "failover_transaction_1"));
    SQLFreeHandle(SQL_HANDLE_STMT, handle);

    EXPECT_EQ(SQL_SUCCESS, SQLAllocHandle(SQL_HANDLE_STMT, dbc, &handle));

    // Clean up test
    SQLRETURN rc = SQLExecDirect(handle, drop_table_query, SQL_NTS);

    EXPECT_EQ(SQL_SUCCESS, rc);
    EXPECT_EQ(SQL_SUCCESS, SQLDisconnect(dbc));
}

/** Writer fails within a transaction. Open transaction with SQLSetConnectAttr */
TEST_F(FailoverIntegrationTest, WriterFailWithinTransaction_setAutoCommitFalse) {
    EXPECT_EQ(SQL_SUCCESS, SQLDriverConnect(dbc, nullptr, AS_SQLTCHAR(default_connection_string.c_str()), SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT));

    // Setup tests
    SQLHSTMT handle;
    EXPECT_EQ(SQL_SUCCESS, SQLAllocHandle(SQL_HANDLE_STMT, dbc, &handle));

    auto drop_table_query = AS_SQLTCHAR("DROP TABLE IF EXISTS failover_transaction_2"); // Setting up tables
    auto create_table_query = AS_SQLTCHAR("CREATE TABLE failover_transaction_2 (id INT NOT NULL PRIMARY KEY, failover_transaction_2_field VARCHAR(255) NOT NULL)");

    // Execute setup query
    EXPECT_TRUE(SQL_SUCCEEDED(SQLExecDirect(handle, drop_table_query, SQL_NTS)));
    EXPECT_EQ(SQL_SUCCESS, SQLExecDirect(handle, create_table_query, SQL_NTS));

    // Set autocommit = false
    EXPECT_EQ(SQL_SUCCESS, SQLSetConnectAttr(dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0));

    // Run insert query in a new transaction
    auto insert_query_a = AS_SQLTCHAR("INSERT INTO failover_transaction_2 VALUES (1, 'test field string 1')");
    EXPECT_EQ(SQL_SUCCESS, SQLExecDirect(handle, insert_query_a, SQL_NTS));

    failover_cluster_and_wait_until_writer_changed(rds_client, cluster_id, writer_id, target_writer_id);

    // If there is an active transaction, roll it back and return an error 08007.
    assert_query_failed(dbc, SERVER_ID_QUERY, ERROR_TRANSACTION_UNKNOWN);

    // Query new ID after failover
    std::string current_connection_id = query_instance_id(dbc);

    // Check if current connection is a new writer
    EXPECT_TRUE(is_DB_instance_writer(rds_client, cluster_id, current_connection_id));
    EXPECT_NE(current_connection_id, writer_id);

    // No rows should have been inserted to the table
    EXPECT_EQ(0, query_count_table_rows(handle, "failover_transaction_2"));
    SQLFreeHandle(SQL_HANDLE_STMT, handle);

    EXPECT_EQ(SQL_SUCCESS, SQLAllocHandle(SQL_HANDLE_STMT, dbc, &handle));

    // Set autocommit = true
    EXPECT_EQ(SQL_SUCCESS, SQLSetConnectAttr(dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0));

    // Clean up test
    SQLRETURN rc = SQLExecDirect(handle, drop_table_query, SQL_NTS);

    EXPECT_EQ(SQL_SUCCESS, rc);
    EXPECT_EQ(SQL_SUCCESS, SQLDisconnect(dbc));
}
