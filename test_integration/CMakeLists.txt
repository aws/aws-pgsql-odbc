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

option(UNICODE_BUILD "Enable Unicode build" OFF)
option(TEST_LIMITLESS "Enable Limitless tests" OFF)
option(SHOULD_BUILD_FAILOVER "Enable Failover tests" OFF)

# Don't build Failover tests if Limitless tests are enabled
if(TEST_LIMITLESS)
  set(SHOULD_BUILD_FAILOVER FALSE)
endif()

# Don't build Limitless tests if Failover tests are enabled
if(SHOULD_BUILD_FAILOVER)
  set(TEST_LIMITLESS FALSE)
endif()

# If TEST_LIMITLESS is undefined, or another falsey value, ensure it is set to FALSE
if (NOT TEST_LIMITLESS)
  set(TEST_LIMITLESS FALSE)
endif()

# If SHOULD_BUILD_FAILOVER is undefined, or another falsey value, ensure it is set to FALSE
if (NOT SHOULD_BUILD_FAILOVER)
  set(SHOULD_BUILD_FAILOVER FALSE)
endif()

# Add UNICODE definition to build so unicode ODBC functions are used
if(UNICODE_BUILD)
  add_definitions(-DUNICODE)
endif()

#-----------------------------------------------------
# Find and add ODBC

if(APPLE)
  # Required to ensure unixodbc is found and used before iODBC
  set(ODBC_INCLUDE_DIR /usr/local/opt/unixodbc/include)
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

if(SHOULD_BUILD_FAILOVER)
  #-----------------------------------------------------
  # AWS SDK for C++
  # The SDK is only required for failover integration tests

  # Paths to search when attempting to find the AWS SDK
  set(AWS_SDK_DIR "${CMAKE_SOURCE_DIR}/../libs/aws-rds-odbc/aws_sdk/install")
  list(APPEND CMAKE_PREFIX_PATH ${AWS_SDK_DIR})
  string(REPLACE ";" "/aws-cpp-sdk-all;" SYSTEM_MODULE_PATH "${CMAKE_SYSTEM_PREFIX_PATH}/aws-cpp-sdk-all")
  list(APPEND CMAKE_PREFIX_PATH ${SYSTEM_MODULE_PATH})

  find_package(AWSSDK REQUIRED COMPONENTS core rds sts)
  message(STATUS "AWSSDK_INCLUDE_DIRS: ${AWSSDK_INCLUDE_DIRS}")
  message(STATUS "AWSSDK_LINK_LIBRARIES: ${AWSSDK_LINK_LIBRARIES}")
  include_directories(${AWSSDK_INCLUDE_DIRS})

  #-----------------------------------------------------
  # macOS specific configuration

  if(APPLE OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # ZLIB needs to be explicity required for AWS SDK
    find_package(ZLIB REQUIRED)
  endif()
endif()

#-----------------------------------------------------

enable_testing()

set(TEST_UTILITIES
  integration_test_utils.cc
  library_utils/host_info.cc
)

set(TEST_SUITE
  connection_string_builder_test.cc
  iam_authentication_integration_test.cc
  secrets_manager_integration_test.cc
)

if(SHOULD_BUILD_FAILOVER)
  list(APPEND TEST_SUITE base_failover_integration_test.cc failover_integration_test.cc)
elseif(TEST_LIMITLESS)
  list(APPEND TEST_SUITE limitless_integration_test.cc)
endif()

set(EXTERNAL_LIBRARIES
  gtest_main
  ${ODBC_LIBRARIES}
)

if(SHOULD_BUILD_FAILOVER)
  list(APPEND EXTERNAL_LIBRARIES ${AWSSDK_LINK_LIBRARIES})
endif()

add_executable(
  # Executable Name
  ${PROJECT_NAME}

  ${TEST_UTILITIES}
  ${TEST_SUITE}
)

include(GoogleTest)

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE

  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/library_utils
)

target_link_libraries(
  # Name
  ${PROJECT_NAME}

  # External Libaries
  ${EXTERNAL_LIBRARIES}
)

set_target_properties(
  ${PROJECT_NAME} 
  PROPERTIES 
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)

#-----------------------------------------------------
# Generate odbc.ini and odbcinst.ini

# Set values to help generate odbc.ini and odbcinst.ini files for Dockerized tests
SET(ODBC_PATH ${CMAKE_SOURCE_DIR}/../testframework/src/test/resources/)
SET(TEST_DRIVER_UNICODE "aws-pgsql-odbc-unicode")
SET(TEST_DRIVER_ANSI "aws-pgsql-odbc-ansi")
SET(DRIVER_PATH_UNICODE $ENV{TEST_DRIVER_UNICODE})
SET(DRIVER_PATH_ANSI $ENV{TEST_DRIVER_ANSI})
SET(DRIVER_DESCRIPTION "AWS PG ODBC Driver for connecting to PG")
SET(THREADING "0")

SET(TEST_DSN_UNICODE $ENV{TEST_DSN_UNICODE}) 
SET(TEST_DSN_ANSI $ENV{TEST_DSN_ANSI})
SET(TEST_DATABASE $ENV{TEST_DATABASE})
SET(DESCRIPTION "AWS PG ODBC Driver for Dockerized tests")
SET(TEST_UID $ENV{TEST_USERNAME})

CONFIGURE_FILE(${ODBC_PATH}/odbc.ini.in     $ENV{ODBCINI} @ONLY)
CONFIGURE_FILE(${ODBC_PATH}/odbcinst.ini.in $ENV{ODBCINST} @ONLY)
