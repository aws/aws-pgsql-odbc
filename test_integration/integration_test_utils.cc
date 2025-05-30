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
