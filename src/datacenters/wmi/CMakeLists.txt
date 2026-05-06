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

find_package(CURL REQUIRED)
find_package(cJSON REQUIRED)

add_library(wmi_device SHARED wmi.c)

target_link_libraries(wmi_device PUBLIC qdmi::qdmi qdmi::qdmi_project_warnings curl
                                        cjson)

generate_prefixed_qdmi_headers(${WMI_QDMI_PREFIX})

target_include_directories(wmi_device
                           PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include)
add_library(qdmi::wmi_device ALIAS wmi_device)
target_compile_definitions(wmi_device PRIVATE MAQCS)

target_compile_options(wmi_device PRIVATE --coverage -fprofile-arcs
                                          -ftest-coverage -O0)
target_link_libraries(wmi_device PRIVATE gcov --coverage)
