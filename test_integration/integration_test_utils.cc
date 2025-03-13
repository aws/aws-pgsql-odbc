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

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include <codecvt>
#include <locale>
#include <string.h>

#include "integration_test_utils.h"

#if defined(__APPLE__) || defined(__linux__)
typedef std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
#else
typedef std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
#endif

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

    memset(&hints, 0, sizeof(hints));
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

SQLWCHAR *INTEGRATION_TEST_UTILS::to_sqlwchar(std::string str) {
    // this is static, such that the array returned is valid until it the function is called again
    // otherwise the std::wstring would be destructed and the life of the character array is undefined
    static std::wstring wstr = converter{}.from_bytes(str);
    return AS_SQLWCHAR(wstr.c_str());
}
