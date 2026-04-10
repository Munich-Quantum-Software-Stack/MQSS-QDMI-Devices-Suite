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
cmake_minimum_required(VERSION 3.19...3.30)

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

set(AVAILABLE_MODULES
    lrz
    eviden
    wmi
    dcdb
    databases
    datacenters
    simulators)
# Main config variable for users
set(BUILD_MODULES
    "all;"
    CACHE STRING "Semicolon-separated list of modules to build (or 'all')")

# Normalize and parse user input
string(TOLOWER "${BUILD_MODULES}" BUILD_MODULES)
string(REPLACE " " "" BUILD_MODULES "${BUILD_MODULES}")
string(REPLACE "," ";" BUILD_MODULES "${BUILD_MODULES}")

set(ENABLED_MODULES "")

if(BUILD_MODULES STREQUAL "all")
  set(ENABLED_MODULES ${AVAILABLE_MODULES})
else()
  foreach(module ${BUILD_MODULES})
    list(FIND AVAILABLE_MODULES ${module} index)
    if(index EQUAL -1)
      message(
        FATAL_ERROR
          "Unknown module: '${module}'. Available modules: ${AVAILABLE_MODULES}"
      )
    endif()
    list(APPEND ENABLED_MODULES ${module})
  endforeach()
endif()

# Show what's being built
message(STATUS "Enabled modules: ${ENABLED_MODULES}")

# Include submodules conditionally
foreach(module ${AVAILABLE_MODULES})
  list(FIND ENABLED_MODULES ${module} index)
  if(NOT index EQUAL -1)
    message(STATUS "Including module: ${module}")
    if(module STREQUAL "lrz")
      option(BUILD_BACKEND_LRZ "Build LRZ Backend" ON)
    elseif(module STREQUAL "eviden")
      option(BUILD_BACKEND_EVIDEN "Build EVIDEN Backend" ON)
    elseif(module STREQUAL "dcdb")
      option(BUILD_BACKEND_DCDB "Build DCDB Backend" ON)
    elseif(module STREQUAL "wmi")
      option(BUILD_BACKEND_WMI "Build WMI Backend" ON)
    elseif(module STREQUAL "simulators")
      option(BUILD_SIMULATOR "Build Simulator Backends" ON)
    elseif(module STREQUAL "datacenters")
      option(BUILD_DATACENTERS "Build Datacenter Backends" ON)
    elseif(module STREQUAL "databases")
      option(BUILD_DATABASES "Build Database Backends" ON)
    endif()
  else()
    message(STATUS "Skipping module: ${module}")
  endif()
endforeach()

if(BUILD_DATACENTERS)
  set(BUILD_BACKEND_LRZ
      ON
      CACHE BOOL "" FORCE)
endif()

if(BUILD_DATABASES)
  set(BUILD_BACKEND_DCDB
      ON
      CACHE BOOL "" FORCE)
endif()

if(BUILD_SIMULATOR)
  set(BUILD_BACKEND_EVIDEN
      ON
      CACHE BOOL "" FORCE)
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

if(BUILD_BACKEND_WMI)
  set(WMI_QDMI_PREFIX "WMI")
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
