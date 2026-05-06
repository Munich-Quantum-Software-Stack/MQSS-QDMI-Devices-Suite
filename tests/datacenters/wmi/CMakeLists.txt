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

enable_testing()
include(GoogleTest)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

generate_device_defs_executable(${WMI_QDMI_PREFIX})

add_executable(WMIBackendTest WMIBackendTest.cpp)

target_link_libraries(WMIBackendTest PRIVATE gtest gtest_main qdmi::qdmi
                                             wmi_device qdmi::qdmi_project_warnings)

add_dependencies(WMIBackendTest qdmi_test_wmi_device_defs)

gtest_discover_tests(WMIBackendTest)

if(ENABLE_COVERAGE)
  target_compile_options(WMIBackendTest INTERFACE --coverage -fprofile-arcs
                                                  -ftest-coverage -O0)
  target_link_libraries(WMIBackendTest INTERFACE gcov --coverage)
endif()
