# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0
# (GPLv2), as published by the Free Software Foundation, with the
# following additional permissions:
#
# This program is distributed with certain software that is licensed
# under separate terms, as designated in a particular file or component
# or in the license documentation. Without limiting your rights under
# the GPLv2, the authors of this program hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with the program.
#
# Without limiting the foregoing grant of rights under the GPLv2 and
# additional permission as to separately licensed software, this
# program is also subject to the Universal FOSS Exception, version 1.0,
# a copy of which can be found along with its FAQ at
# http://oss.oracle.com/licenses/universal-foss-exception.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see 
# http://www.gnu.org/licenses/gpl-2.0.html. 

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

set(TEST_UTILITIES
    connection_string_builder.h
    integration_test_utils.cc
    integration_test_utils.h
)

set(TEST_SUITE
    connection_string_builder_test.cc
    iam_authentication_integration_test.cc
    secrets_manager_integration_test.cc
)

IF (TEST_LIMITLESS)
    list(APPEND TEST_SUITE limitless_integration_test.cc)
endif()

add_executable(
  # Executable Name
  ${PROJECT_NAME}

  ${TEST_UTILITIES}
  ${TEST_SUITE}
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
