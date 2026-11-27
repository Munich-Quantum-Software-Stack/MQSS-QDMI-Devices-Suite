# Copyright (c) 2026 Munich Quantum Valley
#
# Licensed under the Apache License v2.0 with LLVM Exceptions (the "License");
# you may not use this file except in compliance with the License. You may
# obtain a copy of the License at
#
# https://llvm.org/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

enable_testing()
include(GoogleTest)

generate_device_defs_executable(${MQV_QDMI_PREFIX} TARGET mqv_device)

# Unit tests run offline. The end-to-end tests (MQVDeviceE2E) are skipped unless
# MQV_QDMI_E2E_BASEURL is set; mock_mqp/run_e2e.sh runs them against a local
# mock of the MQP-API.
add_executable(MQVBackendTest MQVBackendTest.cpp MQVBackendE2ETest.cpp)

target_link_libraries(
  MQVBackendTest PRIVATE gtest gtest_main qdmi::qdmi mqv_device
                         qdmi::qdmi_project_warnings)
target_compile_features(MQVBackendTest PRIVATE cxx_std_20)
target_compile_definitions(
  MQVBackendTest
  PRIVATE MQV_QDMI_DEVICE_ID="$<TARGET_PROPERTY:mqv_device,QDMI_DEVICE_ID>")

add_dependencies(MQVBackendTest qdmi_test_mqv_device_defs)

gtest_discover_tests(
  MQVBackendTest WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} DISCOVERY_TIMEOUT
                                                               60)
