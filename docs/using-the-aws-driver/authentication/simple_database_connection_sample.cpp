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

#ifdef WIN32
#include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>

#include <iostream>

#define MAX_NAME_LEN 4096
#define MAX_STATE_LENGTH 32
#define QUERY_BUFFER_SIZE 256
#define SQL_MAX_MESSAGE_LENGTH 512

/**
 * Print the error message if the previous ODBC command failed.
 */
void print_error(SQLRETURN rc, SQLHANDLE connection_handle, SQLHANDLE statement_handle) {
	if (SQL_SUCCEEDED(rc)) {
		return;
	}

	SQLSMALLINT stmt_length;
	SQLINTEGER native_error;

	SQLTCHAR sqlstate[MAX_STATE_LENGTH], message[QUERY_BUFFER_SIZE];
	SQLRETURN err_rc =
		SQLError(nullptr, connection_handle, statement_handle, sqlstate, &native_error, message, SQL_MAX_MESSAGE_LENGTH - 1, &stmt_length);

	if (SQL_SUCCEEDED(err_rc)) {
		std::cout << sqlstate << ": " << message << std::endl;
	}
	throw std::runtime_error("An error has occurred while running this sample code.");
}

int main() {
	SQLHENV env;
	SQLHDBC dbc;
	SQLHSTMT stmt;
	SQLSMALLINT len;
	SQLRETURN rc;
	SQLCHAR conn_in[MAX_NAME_LEN], conn_out[MAX_NAME_LEN], query_buffer[QUERY_BUFFER_SIZE];
	SQLTCHAR instance_id[QUERY_BUFFER_SIZE];

	// Setup connection string
	const char* dsn = "my_dsn";
	const char* user = "username";
	const char* password = "password";
	const char* server = "database-pg-name.cluster-XYZ.us-east-2.rds.amazonaws.com";
	int port = 5432;
	const char* db = "postgres";

	sprintf(reinterpret_cast<char*>(conn_in), "DSN=%s;UID=%s;PWD=%s;SERVER=%s;PORT=%d;DATABASE=%s;AuthType=database;", dsn, user, password, server,
			port, db);

	// Setup
	SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &env);
	SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
	SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

	// Connect
	rc = SQLDriverConnect(dbc, nullptr, conn_in, SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);
	print_error(rc, dbc, nullptr);

	// Execute
	rc = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
	sprintf(reinterpret_cast<char*>(query_buffer), "SELECT aurora_db_instance_identifier()");
	rc = SQLExecDirect(stmt, query_buffer, SQL_NTS);
	print_error(rc, nullptr, stmt);

	rc = SQLBindCol(stmt, 1, SQL_C_CHAR, instance_id, sizeof(instance_id), nullptr);
	rc = SQLFetch(stmt);
	print_error(rc, nullptr, stmt);

	std::cout << "Connected to instance: " << reinterpret_cast<char*>(instance_id) << std::endl;

	// Cleanup
	SQLFreeHandle(SQL_HANDLE_STMT, stmt);
	SQLDisconnect(dbc);
	SQLFreeHandle(SQL_HANDLE_DBC, dbc);
	SQLFreeHandle(SQL_HANDLE_ENV, env);
}
