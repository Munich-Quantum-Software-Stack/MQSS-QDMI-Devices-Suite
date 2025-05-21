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

set(DCDB_SOURCE_DIR $ENV{DCDB_SOURCE_LOCATION})
message(${DCDB_SOURCE_DIR})

set(DCDB_LIBRARY "${DCDB_SOURCE_DIR}/lib/libdcdb.so")
set(DCDB_QUERY_OBJECT "${DCDB_SOURCE_DIR}/tools/dcdbquery/query.o")
set(DCDB_INCLUDE_DIRS "${DCDB_SOURCE_DIR}/lib/include")

list(APPEND DCDB_INCLUDE_DIRS "${DCDB_SOURCE_DIR}/common/include")
list(APPEND DCDB_INCLUDE_DIRS "${DCDB_SOURCE_DIR}/../install/include")
list(APPEND DCDB_INCLUDE_DIRS "${DCDB_SOURCE_DIR}/tools/dcdbquery")

enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(dcdb_device SHARED dcdb.cpp session.cpp sensors.cpp)

target_link_libraries(dcdb_device PUBLIC qdmi::qdmi qdmi::project_warnings
                                         ${DCDB_LIBRARY})

generate_prefixed_qdmi_headers(${DCDB_QDMI_PREFIX})

target_include_directories(
  dcdb_device PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include
                     ${CMAKE_CURRENT_SRC_DIR}/include ${DCDB_INCLUDE_DIRS})

add_library(qdmi::dcdb_device ALIAS dcdb_device)

if(ENABLE_COVERAGE)
  target_compile_options(dcdb_device INTERFACE --coverage -fprofile-arcs
                                               -ftest-coverage -O0)
  target_link_libraries(dcdb_device INTERFACE gcov --coverage)
endif()
