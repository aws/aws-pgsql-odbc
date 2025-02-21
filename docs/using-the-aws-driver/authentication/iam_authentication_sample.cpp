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

#ifdef WIN32
#include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>

#include <iostream>

#define MAX_NAME_LEN 4096
#define QUERY_BUFFER_SIZE 256

int main() {
    SQLHENV env;
    SQLHDBC dbc;
    SQLHSTMT stmt;
    SQLSMALLINT len;
    SQLRETURN rc;
    SQLCHAR conn_in[MAX_NAME_LEN], conn_out[MAX_NAME_LEN], query_buffer[QUERY_BUFFER_SIZE];
    SQLTCHAR instance_id[QUERY_BUFFER_SIZE];

    // Setup connection string
    const char *dsn = "my_dsn";
    const char *user = "iam_db_user";
    const char *server = "database-pg-name.cluster-XYZ.us-east-2.rds.amazonaws.com";
    int port = 5432;
    const char *db = "postgres";
    const char* iam_config = "SSLMODE=require;REGION=us-east-2;TOKENEXPIRATION=850";

    sprintf(reinterpret_cast<char *>(conn_in),
            "DSN=%s;UID=%s;SERVER=%s;PORT=%d;DATABASE=%s;%s;",
            dsn, user, server, port, db, iam_config);

    // Setup
    SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &env);
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
    SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

    // Connect
    rc = SQLDriverConnect(dbc, nullptr, conn_in, SQL_NTS, conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);

    // Execute
    rc = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
    sprintf(reinterpret_cast<char *>(query_buffer), "SELECT aurora_db_instance_identifier()");
    rc = SQLExecDirect(stmt, query_buffer, SQL_NTS);
    rc = SQLBindCol(stmt, 1, SQL_C_CHAR, instance_id, sizeof(instance_id), nullptr);
    rc = SQLFetch(stmt);

    std::cout << "Connected to instance: " << reinterpret_cast<char *>(instance_id) << std::endl;

    // Cleanup
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    SQLDisconnect(dbc);
    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
    SQLFreeHandle(SQL_HANDLE_ENV, env);
}
