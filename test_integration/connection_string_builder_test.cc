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

#include <gtest/gtest.h>

#include "connection_string_builder.h"
#include <iostream>

class ConnectionStringBuilderTest : public testing::Test {};

// All connection string fields are set in the builder.
TEST_F(ConnectionStringBuilderTest, test_complete_string) {
    ConnectionStringBuilder builder("testDSN", "testServer", 5432);
    const std::string connection_string = builder.withUID("testUser")
                                              .withPWD("testPwd")
                                              .withDatabase("testDb")
                                              .withLogDir("/temp/logs")
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
        "DSN=testDSN;SERVER=testServer;PORT=5432;UID=testUser;PWD=testPwd;DATABASE=testDb;LOGDIR=/temp/logs;"
        "ENABLECLUSTERFAILOVER=1;FAILOVERMODE=STRICT_WRITER;READERHOSTSELECTORSTRATEGY=RANDOM;IGNORETOPOLOGYREQUEST=1;TOPOLOGYHIGHREFRESHRATE=2;TOPOLOGYREFRESHRATE=3;FAILOVERTIMEOUT=120000;HOSTPATTERN=?.testDomain;"
        "AUTHTYPE=IAM;REGION=us-east-1;IAMHOST=domain;TOKENEXPIRATION=123;SECRETID=secret;SSLMODE=prefer;"
        "LIMITLESSENABLED=1;LIMITLESSMODE=lazy;LIMITLESSMONITORINTERVALMS=234;LIMITLESSSERVICEID=limitless;";
    EXPECT_EQ(0, expected.compare(connection_string));
}

// No optional fields are set in the builder. Build will succeed. Connection string with required fields.
TEST_F(ConnectionStringBuilderTest, test_only_required_fields) {
    ConnectionStringBuilder builder("testDSN", "testServer", 5432);
    const std::string connection_string = builder.getString();

    const std::string expected = "DSN=testDSN;SERVER=testServer;PORT=5432;";
    EXPECT_EQ(0, expected.compare(connection_string));
}

// Some optional fields are set and others not set in the builder. Build will succeed.
// Connection string with required fields and ONLY the fields that were set.
TEST_F(ConnectionStringBuilderTest, test_some_optional) {
    ConnectionStringBuilder builder("testDSN", "testServer", 5432);
    const std::string connection_string = builder.withUID("testUser").withPWD("testPwd").getString();

    const std::string expected("DSN=testDSN;SERVER=testServer;PORT=5432;UID=testUser;PWD=testPwd;");
    EXPECT_EQ(0, expected.compare(connection_string));
}

// Boolean values are set in the builder. Build will succeed. True will be marked as 1 in the string, false 0.
TEST_F(ConnectionStringBuilderTest, test_setting_boolean_fields) {
    ConnectionStringBuilder builder("testDSN", "testServer", 5432);
    const std::string connection_string = builder.withUID("testUser")
                                              .withPWD("testPwd")
                                              .withEnableClusterFailover(false)
                                              .withLimitlessEnabled(false)
                                              .getString();

    const std::string expected(
        "DSN=testDSN;SERVER=testServer;PORT=5432;UID=testUser;PWD=testPwd;ENABLECLUSTERFAILOVER=0;LIMITLESSENABLED=0;");
    EXPECT_EQ(0, expected.compare(connection_string));
}
