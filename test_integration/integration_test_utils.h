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

#include <string>

#include <sql.h>
#include <sqlext.h>
#include <gtest/gtest.h>

#include "string_helper.h"

#define MAX_NAME_LEN 4096
#define SQL_MAX_MESSAGE_LENGTH 512

class INTEGRATION_TEST_UTILS {
public:
    static char* get_env_var(const char* key, char* default_value);
    static int str_to_int(const char* str);
    static std::string host_to_IP(std::string hostname);
    static void print_errors(SQLHANDLE handle, int32_t handle_type);
    static SQLRETURN exec_query(SQLHSTMT stmt, char *query_buffer);
	static void clear_memory(void* dest, size_t count);
};

#endif // INTEGRATIONTESTUTILS_H_
