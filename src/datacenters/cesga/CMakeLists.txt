# Copyright 2025 Munich Quantum Software Stack Project / CESGA
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# ------------------------------------------------------------------------------

add_library(cesga_device SHARED cesga.c)

target_link_libraries(cesga_device PUBLIC qdmi::qdmi qdmi::project_warnings)

generate_prefixed_qdmi_headers(${CESGA_QDMI_PREFIX})

target_include_directories(cesga_device
                           PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include)
add_library(qdmi::cesga_device ALIAS cesga_device)

if(ENABLE_COVERAGE)
  target_compile_options(cesga_device INTERFACE --coverage -fprofile-arcs
                                                -ftest-coverage -O0)
  target_link_libraries(cesga_device INTERFACE gcov --coverage)
endif()
