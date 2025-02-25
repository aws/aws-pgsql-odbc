# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
#
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.22 FATAL_ERROR)
project("integration")
set(CMAKE_CXX_STANDARD 20)

#-----------------------------------------------------
# Find and add ODBC

if(APPLE)
  # Required to ensure unixodbc is found and used before iODBC
  set(ODBC_INCLUDE_DIR /opt/homebrew/opt/unixodbc/include)
endif()

find_package(ODBC REQUIRED)

message(STATUS "ODBC_INCLUDE_DIRS: ${ODBC_INCLUDE_DIRS}")
message(STATUS "ODBC_LIBRARIES: ${ODBC_LIBRARIES}")
include_directories(${ODBC_INCLUDE_DIRS})

#-----------------------------------------------------
# Google Test

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)

FetchContent_MakeAvailable(googletest)

enable_testing()

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

#-----------------------------------------------------
# Toxiproxy - Simulate Network Outages

FetchContent_Declare(
  toxiproxy
  GIT_REPOSITORY https://github.com/Bit-Quill/toxiproxy-cpp
  GIT_TAG main
)

FetchContent_MakeAvailable(toxiproxy)

#-----------------------------------------------------

enable_testing()

add_executable(
  # Executable Name
  ${PROJECT_NAME}

  # Runner / Utility
  connection_string_builder.h
  integration_test_utils.cc
  integration_test_utils.h

  # Test Suites
  connection_string_builder_test.cc
  iam_authentication_integration_test.cc
  secrets_manager_integration_test.cc
)

include(GoogleTest)

target_link_libraries(
  # Name
  ${PROJECT_NAME}

  # External Libaries
  gtest_main
  toxiproxy
  ${ODBC_LIBRARIES}
)

set_target_properties(
  ${PROJECT_NAME} 
  PROPERTIES 
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)

# Set values to help generate odbc.ini and odbcinst.ini files for Dockerized tests
SET(ODBC_PATH ${CMAKE_SOURCE_DIR}/../testframework/src/test/resources/)
SET(TEST_DRIVER "aws-pgsql-odbc")
SET(DRIVER_PATH $ENV{TEST_DRIVER})
SET(DRIVER_DESCRIPTION "AWS PG ODBC Driver for connecting to PG")
SET(THREADING "0")

SET(TEST_DSN $ENV{TEST_DSN})
SET(TEST_DATABASE $ENV{TEST_DATABASE})
SET(DESCRIPTION "AWS PG ODBC Driver for Dockerized tests")
SET(TEST_UID $ENV{TEST_USERNAME})

CONFIGURE_FILE(${ODBC_PATH}/odbc.ini.in     $ENV{ODBCINI} @ONLY)
CONFIGURE_FILE(${ODBC_PATH}/odbcinst.ini.in $ENV{ODBCINST} @ONLY)
