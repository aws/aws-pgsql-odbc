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

#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <chrono>
#include <future>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std;

const static wstring LIMITLESS_DSN_ENVVAR = L"LIMITLESS_DSN";
const static wstring NUM_THREADS_ENVVAR = L"NUM_THREADS";
const static wstring NUM_LOOPS_ENVVAR = L"NUM_LOOPS";

static wstring DSN = L"";
static wstring CONNECTION_STRING = L"";
static int NUM_THREADS = 1;
static int NUM_LOOPS = 10;

#ifdef _DEBUG
bool DEBUG = true;
#else
bool DEBUG = false;
#endif // _DEBUG

wstring GetEnvironmentVariableValue(const wstring& variableName) {
    DWORD buffer_size = GetEnvironmentVariableW(variableName.c_str(), NULL, 0);
    if (buffer_size == 0) {
        return L"";
    }

    wstring value(buffer_size, L'\0');
    DWORD result_size = GetEnvironmentVariableW(variableName.c_str(), &value[0], buffer_size);
    if (result_size == 0) {
        return L"";
    }

    // Remove the null terminator before returning
    value.resize(result_size);
    return value;
}

void GetConfigFromEnvVars() {
    DSN = GetEnvironmentVariableValue(LIMITLESS_DSN_ENVVAR);
    if (DSN.empty()) {
        wcerr << L"The required environment variable LIMITLESS_DSN is missing or empty\n";
        exit(1);
    }
    CONNECTION_STRING = L"DSN=" + DSN + L";";

    wstring num_threads_str = GetEnvironmentVariableValue(NUM_THREADS_ENVVAR);
    if (!num_threads_str.empty()) {
        NUM_THREADS = stoi(num_threads_str);
    }
    
    wstring num_loops_str = GetEnvironmentVariableValue(NUM_LOOPS_ENVVAR);
    if (!num_loops_str.empty()) {
        NUM_LOOPS = stoi(num_loops_str);
    }
}

void Cleanup(SQLHENV hEnv, SQLHDBC hDbc, SQLHSTMT hStmt) {
    if (hStmt != NULL) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    }
    if (hDbc != NULL) {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    }
    if (hEnv != NULL) {
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
    }
}

void ExecuteQuery() {
    SQLHENV hEnv = NULL;
    SQLHDBC hDbc = NULL;
    SQLHSTMT hStmt = NULL;
    SQLRETURN ret;
    SQLWCHAR query[] = L"select aurora_db_instance_identifier()";

    // Allocate environment handle
    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    if (ret != SQL_SUCCESS) {
        wcerr << L"SQLAllocHandle failed\n";
        return;
    }

    // Set the ODBC environment to version 3
    ret = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    if (ret != SQL_SUCCESS) {
        wcerr << L"SQLSetEnvAttr failed\n";
        Cleanup(hEnv, hDbc, hStmt);
        return;
    }

    // Allocate connection handle
    ret = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
    if (ret != SQL_SUCCESS) {
        wcerr << L"SQLAllocHandle failed\n";
        Cleanup(hEnv, hDbc, hStmt);
        return;
    }

    // Connect to the database
    SQLWCHAR outConnectionString[1024] = {};
    SQLSMALLINT outStringLength = 0;
    ret = SQLDriverConnectW(
        hDbc, NULL, (SQLWCHAR*)CONNECTION_STRING.c_str(), SQL_NTS,
        outConnectionString, sizeof(outConnectionString) / sizeof(SQLWCHAR),
        &outStringLength, SQL_DRIVER_COMPLETE
    );
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        wcerr << L"SQLDriverConnectW failed\n";
        Cleanup(hEnv, hDbc, hStmt);
        return;
    }

    // Allocate statement handle
    ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    if (ret != SQL_SUCCESS) {
        wcerr << L"SQLAllocHandle failed\n";
        Cleanup(hEnv, hDbc, hStmt);
        return;
    }

    // Execute SQL query
    ret = SQLExecDirect(hStmt, query, SQL_NTS);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        wcerr << L"SQLExecDirect failed\n";
        Cleanup(hEnv, hDbc, hStmt);
        return;
    }

    // Retrieve the result
    SQLCHAR columnData[256];
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        ret = SQLGetData(hStmt, 1, SQL_C_CHAR, columnData, sizeof(columnData), NULL);
        if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
            if (DEBUG) {
                cout << "Router: " << columnData << "\n";
            }
        }
        else {
            wcerr << L"SQLGetData failed\n";
            Cleanup(hEnv, hDbc, hStmt);
            return;
        }
    }

    Cleanup(hEnv, hDbc, hStmt);
}

int PerformanceWorker() {
    int total_execution_time = 0;

    for (int i = 0; i < NUM_LOOPS; ++i) {
        auto start = chrono::high_resolution_clock::now();
        ExecuteQuery();
        auto end = chrono::high_resolution_clock::now();
        total_execution_time += static_cast<int>(chrono::duration_cast<chrono::milliseconds>(end - start).count());
    }

    int average_execution_time_in_ms = total_execution_time / NUM_LOOPS;
    return average_execution_time_in_ms;
}

int main() {
    GetConfigFromEnvVars();

    vector<future<int>> threads;
    try {
        wcout << L"Starting " << NUM_THREADS << L" performance workers with " << NUM_LOOPS << " loops in each thread\n";
        for (int i = 0; i < NUM_THREADS; ++i) {
            if (DEBUG) {
                wcout << L"Starting thread ID: " << i << "\n";
            }
            threads.push_back(async(launch::async, PerformanceWorker));
        }
    } catch (const std::exception& ex) {
        wcerr << L"Unhandled exception in main: " << ex.what() << "\n";
        return 3;
    } catch (...) {
        wcerr << L"Unknown unhandled exception in main.\n";
        return 3;
    }

    std::vector<int> performance_results;
    for (auto& thread : threads) {
        performance_results.push_back(thread.get());
    }

    wcout << L"All performance workers have completed.\n";

    wcout << L"Average query execution times from all performance workers:\n";
    for (size_t i = 0; i < performance_results.size(); ++i) {
        wcout << L"Performance worker " << i + 1 << L": " << performance_results[i] << L" ms\n";
    }
}
