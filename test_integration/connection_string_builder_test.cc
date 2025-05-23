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

#ifdef WIN32
#include <windows.h>
#endif
#include "connection_string_builder.h"

class ConnectionStringBuilderTest : public testing::Test {};

// All connection string fields are set in the builder.
TEST_F(ConnectionStringBuilderTest, test_complete_string) {
    ConnectionStringBuilder builder("testDSN", "testServer", 5432);
    auto connection_string = builder.withUID("testUser")
                                              .withPWD("testPwd")
                                              .withDatabase("testDb")
                                              .withEnableClusterFailover(true)
                                              .withFailoverMode("STRICT_WRITER")
                                              .withReaderHostSelectorStrategy("RANDOM")
                                              .withIgnoreTopologyRequest(1)
                                              .withTopologyHighRefreshRate(2)
                                              .withTopologyRefreshRate(3)
                                              .withFailoverTimeout(120000)
                                              .withHostPattern("?.testDomain")
                                              .withAuthMode("IAM")
                                              .withAuthRegion("us-east-1")
                                              .withAuthHost("domain")
                                              .withAuthExpiration(123)
                                              .withSecretId("secret")
                                              .withSslMode("prefer")
                                              .withLimitlessEnabled(true)
                                              .withLimitlessMode("lazy")
                                              .withLimitlessMonitorIntervalMs(234)
                                              .withLimitlessServiceId("limitless")
                                              .getString();

    const std::string expected =
        "DSN=testDSN;SERVER=testServer;PORT=5432;COMMLOG=1;DEBUG=1;LOGDIR=logs/;RDSLOGTHRESHOLD=0;UID=testUser;PWD=testPwd;DATABASE=testDb;"
        "ENABLECLUSTERFAILOVER=1;FAILOVERMODE=STRICT_WRITER;READERHOSTSELECTORSTRATEGY=RANDOM;IGNORETOPOLOGYREQUEST=1;TOPOLOGYHIGHREFRESHRATE=2;TOPOLOGYREFRESHRATE=3;FAILOVERTIMEOUT=120000;HOSTPATTERN=?.testDomain;"
        "AUTHTYPE=IAM;REGION=us-east-1;IAMHOST=domain;TOKENEXPIRATION=123;SECRETID=secret;SSLMODE=prefer;"
        "LIMITLESSENABLED=1;LIMITLESSMODE=lazy;LIMITLESSMONITORINTERVALMS=234;LIMITLESSSERVICEID=limitless;";

    #ifdef UNICODE
    EXPECT_EQ(0, expected.compare(StringHelper::ToString(connection_string)));
    #else
    EXPECT_EQ(0, expected.compare(connection_string));
    #endif
}

// No optional fields are set in the builder. Build will succeed. Connection string with required fields.
TEST_F(ConnectionStringBuilderTest, test_only_required_fields) {
    ConnectionStringBuilder builder("testDSN", "testServer", 5432);
    auto connection_string = builder.getString();

    const std::string expected = "DSN=testDSN;SERVER=testServer;PORT=5432;COMMLOG=1;DEBUG=1;LOGDIR=logs/;RDSLOGTHRESHOLD=0;";
    #ifdef UNICODE
    EXPECT_EQ(0, expected.compare(StringHelper::ToString(connection_string)));
    #else
    EXPECT_EQ(0, expected.compare(connection_string));
    #endif
}

// Some optional fields are set and others not set in the builder. Build will succeed.
// Connection string with required fields and ONLY the fields that were set.
TEST_F(ConnectionStringBuilderTest, test_some_optional) {
    ConnectionStringBuilder builder("testDSN", "testServer", 5432);
    auto connection_string = builder.withUID("testUser").withPWD("testPwd").getString();

    const std::string expected("DSN=testDSN;SERVER=testServer;PORT=5432;COMMLOG=1;DEBUG=1;LOGDIR=logs/;RDSLOGTHRESHOLD=0;UID=testUser;PWD=testPwd;");
    #ifdef UNICODE
    EXPECT_EQ(0, expected.compare(StringHelper::ToString(connection_string)));
    #else
    EXPECT_EQ(0, expected.compare(connection_string));
    #endif
}

// Boolean values are set in the builder. Build will succeed. True will be marked as 1 in the string, false 0.
TEST_F(ConnectionStringBuilderTest, test_setting_boolean_fields) {
    ConnectionStringBuilder builder("testDSN", "testServer", 5432);
    auto connection_string = builder.withUID("testUser")
                                              .withPWD("testPwd")
                                              .withEnableClusterFailover(false)
                                              .withLimitlessEnabled(false)
                                              .getString();

    const std::string expected(
        "DSN=testDSN;SERVER=testServer;PORT=5432;COMMLOG=1;DEBUG=1;LOGDIR=logs/;RDSLOGTHRESHOLD=0;UID=testUser;PWD=testPwd;ENABLECLUSTERFAILOVER=0;LIMITLESSENABLED=0;");
    #ifdef UNICODE
    EXPECT_EQ(0, expected.compare(StringHelper::ToString(connection_string)));
    #else
    EXPECT_EQ(0, expected.compare(connection_string));
    #endif
}
