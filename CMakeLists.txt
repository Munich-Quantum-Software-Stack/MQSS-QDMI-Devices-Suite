# ------------------------------------------------------------------------------
# Copyright 2024 Munich Quantum Software Stack Project
#
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
# "License"); you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# https://github.com/Munich-Quantum-Software-Stack/QDMI-Devices/blob/develop/LICENSE
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# ------------------------------------------------------------------------------

# set required cmake version
cmake_minimum_required(VERSION 3.24..4.2)

project(
  Backends
  VERSION 0.1
  DESCRIPTION "QDMI Backends"
  LANGUAGES C)

# Generate compile_commands.json to make it easier to work with clang based
# tools
set(CMAKE_EXPORT_COMPILE_COMMANDS
    ON
    CACHE BOOL "Export compile commands" FORCE)

option(BUILD_BACKEND_TESTS "Build the backend tests" ON)
option(BUILD_DOCUMENTATION "Build the documentations" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Available modules
set(AVAILABLE_MODULES lrz eviden dcdb)

# Main config variable for users
set(BUILD_MODULE
    ""
    CACHE STRING "Module to build (choose one of: ${AVAILABLE_MODULES})")

# Normalize user input
string(TOLOWER "${BUILD_MODULE}" BUILD_MODULE)
string(STRIP "${BUILD_MODULE}" BUILD_MODULE)

# Validate module
if(NOT BUILD_MODULE IN_LIST AVAILABLE_MODULES)
  message(
    FATAL_ERROR
      "Unknown or missing module: '${BUILD_MODULE}'. Available modules: ${AVAILABLE_MODULES}"
  )
endif()

# Enable only the chosen module
set(ENABLED_MODULES ${BUILD_MODULE})
message(STATUS "Enabled module: ${ENABLED_MODULES}")

# Include the chosen module
if(BUILD_MODULE STREQUAL "lrz")
  option(BUILD_BACKEND_LRZ "Build LRZ Backend" ON)
elseif(BUILD_MODULE STREQUAL "eviden")
  option(BUILD_BACKEND_EVIDEN "Build EVIDEN Backend" ON)
elseif(BUILD_MODULE STREQUAL "dcdb")
  option(BUILD_BACKEND_DCDB "Build DCDB Backend" ON)
endif()

if(BUILD_BACKEND_EVIDEN)
  set(QAPTIVA_QDMI_PREFIX "QAPTIVA")
endif()

if(BUILD_BACKEND_DCDB)
  set(DCDB_QDMI_PREFIX "DCDB")
endif()

if(BUILD_BACKEND_LRZ)
  set(LRZ_QDMI_PREFIX "LRZ")
endif()

if(ENABLE_COVERAGE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -O0 -g")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -O0 -g")
endif()
include(cmake/ExternalDependencies.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_EXTENSIONS False)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src)

if(BUILD_BACKEND_TESTS)
  add_subdirectory(tests)
endif()

if(BUILD_DOCUMENTATION)
  add_subdirectory(docs)
endif()
