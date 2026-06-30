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

find_package(mqss_client REQUIRED)

add_library(lrz_device SHARED lrz.cpp)

fetch_qdmi("Munich-Quantum-Software-Stack" "v1.3.1")

target_link_libraries(lrz_device PUBLIC qdmi::qdmi qdmi::qdmi_project_warnings
                                        mqss_client)

generate_prefixed_qdmi_headers(${LRZ_QDMI_PREFIX})

target_include_directories(
  lrz_device PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include
                    ${MQSS_CLIENT_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})
add_library(qdmi::lrz_device ALIAS lrz_device)

target_compile_options(lrz_device PRIVATE --coverage -fprofile-arcs
                                          -ftest-coverage -O0)
target_link_libraries(lrz_device PRIVATE gcov --coverage)
