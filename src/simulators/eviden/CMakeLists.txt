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

find_package(Python3 3.10...3.12 REQUIRED COMPONENTS Interpreter Development)

add_library(qaptiva_device SHARED qaptiva.c)

fetch_qdmi("Munich-Quantum-Software-Stack" "v1.3.1")

target_link_libraries(
  qaptiva_device PUBLIC qdmi::qdmi qdmi::qdmi_project_warnings
                        ${Python3_LIBRARIES})
generate_prefixed_qdmi_headers(${QAPTIVA_QDMI_PREFIX})
target_include_directories(
  qaptiva_device PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include
                        ${Python3_INCLUDE_DIRS})
add_library(qdmi::qaptiva_device ALIAS qaptiva_device)

if(ENABLE_COVERAGE)
  target_compile_options(qaptiva_device INTERFACE --coverage -fprofile-arcs
                                                  -ftest-coverage -O0)
  target_link_libraries(qaptiva_device INTERFACE gcov --coverage)
endif()
