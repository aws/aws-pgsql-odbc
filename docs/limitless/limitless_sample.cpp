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
#define SQL_MAX_MESSAGE_LENGTH 512
#define QUERY_BUFFER_SIZE 256

int main() {
    // Setup connection string
    std::string connection_string = "DSN=my_dsn;"
        "SERVER=database-pg-name.cluster-XYZ.us-east-2.rds.amazonaws.com;"
        "PORT=5432;"
        "UID=my_username;"
        "PWD=my_password;"
        "DATABASE=my_database;"
        "ENABLELIMITLESS=1;"
        "LIMITLESSMODE=immediate;"
        "LIMITLESSMONITORINTERVALMS=1000;"
        "LIMITLESSSERVICEID=my-limitless-sample;"
    ;

    // Setup
    SQLHENV env = nullptr;
    SQLHDBC dbc = nullptr;
    SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &env);
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
    SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

    // Connect
    SQLCHAR conn_out[4096] = "\0";
    SQLSMALLINT len;
    SQLDriverConnect(dbc, nullptr, AS_SQLCHAR(connection_string.c_str()), SQL_NTS,
        conn_out, MAX_NAME_LEN, &len, SQL_DRIVER_NOPROMPT);

    // Execute
    char query_buffer[QUERY_BUFFER_SIZE];
    SQLHSTMT stmt = nullptr;
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
    sprintf(query_buffer, "SELECT 1;");
    SQLExecDirect(stmt, AS_SQLCHAR(query_buffer), SQL_NTS);

    // Cleanup
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    SQLDisconnect(dbc);
    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
    SQLFreeHandle(SQL_HANDLE_ENV, env);
}
