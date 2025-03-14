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

#ifndef INTEGRATIONTESTUTILS_H_
#define INTEGRATIONTESTUTILS_H_

#include <sql.h>
#include <sqlext.h>

#define MAX_NAME_LEN 4096
#define SQL_MAX_MESSAGE_LENGTH 512

#define AS_SQLCHAR(str)     const_cast<SQLCHAR*>(reinterpret_cast<const SQLCHAR*>(str))
#define AS_SQLWCHAR(str)    const_cast<SQLWCHAR*>(reinterpret_cast<const SQLWCHAR*>(str))
#define AS_STRING(str)      std::string(reinterpret_cast<char*>(str))
#define AS_WSTRING(str)     std::wstring(reinterpret_cast<wchar_t*>(str))

class INTEGRATION_TEST_UTILS {
public:
    static char* get_env_var(const char* key, char* default_value);
    static int str_to_int(const char* str);
    static std::string host_to_IP(std::string hostname);
    static std::wstring to_wstring(std::string str);
    static SQLWCHAR *to_sqlwchar(std::string str);
    static std::string to_string(std::wstring str);
    static void print_errors(SQLHANDLE handle, int32_t handle_type);
};

#endif // INTEGRATIONTESTUTILS_H_
