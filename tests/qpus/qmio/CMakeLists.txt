# Copyright 2025 Munich Quantum Software Stack Project / CESGA
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# ------------------------------------------------------------------------------

enable_testing()
include(GoogleTest)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

generate_device_defs_executable(${QMIO_QDMI_PREFIX})

add_executable(QmioBackendTest QmioBackendTest.cpp)

target_link_libraries(QmioBackendTest PRIVATE gtest gtest_main qdmi::qdmi
                                              qmio_device qdmi::project_warnings)

add_dependencies(QmioBackendTest qdmi_test_qmio_device_defs)

gtest_discover_tests(QmioBackendTest DISCOVERY_TIMEOUT 60)

if(ENABLE_COVERAGE)
  target_compile_options(QmioBackendTest INTERFACE --coverage -fprofile-arcs
                                                    -ftest-coverage -O0)
  target_link_libraries(QmioBackendTest INTERFACE gcov --coverage)
endif()
