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

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netdb.h>
/* Below adds memset_s if available */
#ifdef __STDC_LIB_EXT1__
#define __STDC_WANT_LIB_EXT1__ 1
#include <string.h> // memset_s
#endif  /* __STDC_WANT_LIB_EXT1__ */
#endif

#include <cmath> // std::round
#include <cstdlib> // std::strtod

#include "integration_test_utils.h"

#define ROUTER_ENDPOINT_LENGTH  2049
#define LOAD_LENGTH             5
#define WEIGHT_SCALING          10
#define MAX_WEIGHT              10
#define MIN_WEIGHT              1

SQLTCHAR* limitless_router_endpoint_query = AS_SQLTCHAR(TEXT("SELECT router_endpoint, load FROM aurora_limitless_router_endpoints()"));

char* INTEGRATION_TEST_UTILS::get_env_var(const char* key, char* default_value) {
    char* value = std::getenv(key);
    if (value == nullptr || value == "") {
        return default_value;
    }

    return value;
}

int INTEGRATION_TEST_UTILS::str_to_int(const char* str) {
    const long int x = strtol(str, nullptr, 10);
    assert(x <= INT_MAX);
    assert(x >= INT_MIN);
    return static_cast<int>(x);
}

double INTEGRATION_TEST_UTILS::str_to_double(const char* str) {
    char* endptr;
    errno = 0;

    const double val = std::strtod(str, &endptr);
    if (errno != 0) {
        std::cerr << "Got the following error number from strtod: " << errno << std::endl;
    }

    if (*endptr != '\0') {
        std::cerr << "There is a non-alphanumerical error passed in to strtod" << std::endl;
    }
    return val;
}

std::string INTEGRATION_TEST_UTILS::host_to_IP(std::string hostname) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif
    int status;
    struct addrinfo hints;
    struct addrinfo* servinfo;
    struct addrinfo* p;
    char ipstr[INET_ADDRSTRLEN];

    clear_memory(&hints, sizeof(hints));
    hints.ai_family = AF_INET; //IPv4
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(hostname.c_str(), NULL, &hints, &servinfo)) != 0) {
        ADD_FAILURE() << "The IP address of host " << hostname << " could not be determined."
            << "getaddrinfo error:" << gai_strerror(status);
        return {};
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        void* addr;

        struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
        addr = &(ipv4->sin_addr);
        inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
    }

    freeaddrinfo(servinfo);
    return std::string(ipstr);
}

void INTEGRATION_TEST_UTILS::print_errors(SQLHANDLE handle, int32_t handle_type) {
    SQLTCHAR    sqlstate[6];
    SQLTCHAR    message[1024];
    SQLINTEGER  nativeerror;
    SQLSMALLINT textlen;
    SQLRETURN   ret;
    SQLSMALLINT recno = 0;

    do {
        recno++;
        ret = SQLGetDiagRec(handle_type, handle, recno, sqlstate, &nativeerror,
                            message, sizeof(message), &textlen);
        if (ret == SQL_INVALID_HANDLE) {
            std::cerr << "Invalid handle" << std::endl;
        } else if (SQL_SUCCEEDED(ret)) {
            #ifdef UNICODE
            std::cerr << StringHelper::ToString(sqlstate) << ": " << StringHelper::ToString(message) << std::endl;
            #else
            std::cerr << sqlstate << ": " << message << std::endl;
            #endif
        }
    } while (ret == SQL_SUCCESS);
}

SQLRETURN INTEGRATION_TEST_UTILS::exec_query(SQLHSTMT stmt, char *query_buffer) {
    #ifdef UNICODE
    std::wstring wquery_buffer = StringHelper::ToWstring(query_buffer);
    SQLTCHAR* query = AS_SQLTCHAR(wquery_buffer.c_str());
    #else
    SQLTCHAR* query = AS_SQLTCHAR(query_buffer);
    #endif
    return SQLExecDirect(stmt, query, SQL_NTS);
}

void INTEGRATION_TEST_UTILS::clear_memory(void* dest, size_t count) {
    #ifdef _WIN32
    SecureZeroMemory(dest, count);
    #else
    #ifdef __STDC_LIB_EXT1__
    memset_s(dest, count, '\0', count);
    #else
    memset(dest, '\0', count);
    #endif /* __STDC__LIB_EXT1__*/
    #endif /* _WIN32 */
return;
}

void INTEGRATION_TEST_UTILS::odbc_cleanup(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt) {
    if (SQL_NULL_HANDLE != hstmt) {
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = SQL_NULL_HSTMT;
    }
    if (SQL_NULL_HANDLE != hdbc) {
        SQLDisconnect(hdbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
        hdbc = SQL_NULL_HDBC;
    }
    if (SQL_NULL_HANDLE != henv) {
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
        henv = SQL_NULL_HENV;
    }
}

static HostInfo create_host(const SQLCHAR* load, const SQLCHAR* router_endpoint, const int host_port_to_map) {
    int64_t weight = std::round(WEIGHT_SCALING - (INTEGRATION_TEST_UTILS::str_to_double(reinterpret_cast<const char *>(load)) * WEIGHT_SCALING));

    if (weight < MIN_WEIGHT || weight > MAX_WEIGHT) {
        weight = MIN_WEIGHT;
        std::cerr << "Invalid router load of " << load << " for " << router_endpoint << std::endl;
    }

    std::string router_endpoint_str(reinterpret_cast<const char *>(router_endpoint));

    return HostInfo(
        router_endpoint_str,
        host_port_to_map,
        UP,
        true,
        nullptr,
        weight
    );
}

std::vector<HostInfo> INTEGRATION_TEST_UTILS::query_for_limitless_routers(SQLHDBC conn, int host_port_to_map) {
    HSTMT hstmt = SQL_NULL_HSTMT;
    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
    if (!SQL_SUCCEEDED(rc)) {
        INTEGRATION_TEST_UTILS::print_errors(conn, SQL_HANDLE_DBC);
        return std::vector<HostInfo>();
    }

    // Generally accepted URL endpoint max length + 1 for null terminator
    SQLCHAR router_endpoint_value[ROUTER_ENDPOINT_LENGTH] = {0};
    SQLLEN ind_router_endpoint_value = 0;

    SQLCHAR load_value[LOAD_LENGTH] = {0};
    SQLLEN ind_load_value = 0;

    rc = SQLBindCol(hstmt, 1, SQL_C_CHAR, &router_endpoint_value, sizeof(router_endpoint_value), &ind_router_endpoint_value);
    SQLRETURN rc2 = SQLBindCol(hstmt, 2, SQL_C_CHAR, &load_value, sizeof(load_value), &ind_load_value);
    if (!SQL_SUCCEEDED(rc) || !SQL_SUCCEEDED(rc2)) {
        INTEGRATION_TEST_UTILS::print_errors(hstmt, SQL_HANDLE_STMT);
        INTEGRATION_TEST_UTILS::odbc_cleanup(SQL_NULL_HENV, SQL_NULL_HDBC, hstmt);
        return std::vector<HostInfo>();
    }

    rc = SQLExecDirect(hstmt, limitless_router_endpoint_query, SQL_NTS);
    if (!SQL_SUCCEEDED(rc)) {
        INTEGRATION_TEST_UTILS::print_errors(hstmt, SQL_HANDLE_STMT);
        INTEGRATION_TEST_UTILS::odbc_cleanup(SQL_NULL_HENV, SQL_NULL_HDBC, hstmt);
        return std::vector<HostInfo>();
    }

    SQLLEN row_count = 0;
    rc = SQLRowCount(hstmt, &row_count);
    if (!SQL_SUCCEEDED(rc)) {
        INTEGRATION_TEST_UTILS::print_errors(hstmt, SQL_HANDLE_STMT);
        INTEGRATION_TEST_UTILS::odbc_cleanup(SQL_NULL_HENV, SQL_NULL_HDBC, hstmt);
        return std::vector<HostInfo>();
    }
    std::vector<HostInfo> limitless_routers;

    while (SQL_SUCCEEDED(rc = SQLFetch(hstmt))) {
        limitless_routers.push_back(create_host(load_value, router_endpoint_value, host_port_to_map));
    }

    INTEGRATION_TEST_UTILS::odbc_cleanup(SQL_NULL_HENV, SQL_NULL_HDBC, hstmt);

    return limitless_routers;
}
