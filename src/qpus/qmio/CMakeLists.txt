# Copyright 2025 Munich Quantum Software Stack Project / CESGA
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# ------------------------------------------------------------------------------

find_package(Python3 3.10...3.12 REQUIRED COMPONENTS Interpreter Development)

add_library(qmio_device SHARED qmio.c)

target_link_libraries(qmio_device PUBLIC qdmi::qdmi qdmi::project_warnings
                                         ${Python3_LIBRARIES})

generate_prefixed_qdmi_headers(${QMIO_QDMI_PREFIX})

target_include_directories(
  qmio_device PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include
                     ${Python3_INCLUDE_DIRS})
add_library(qdmi::qmio_device ALIAS qmio_device)

if(ENABLE_COVERAGE)
  target_compile_options(qmio_device INTERFACE --coverage -fprofile-arcs
                                               -ftest-coverage -O0)
  target_link_libraries(qmio_device INTERFACE gcov --coverage)
endif()

# Copy the auxiliary Python module next to the build output so
# QMIO_AUXILIARY_SCRIPT_LOCATION can simply point at the build dir.
add_custom_command(
  TARGET qmio_device
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/qmio_auxiliary.py
          $<TARGET_FILE_DIR:qmio_device>/qmio_auxiliary.py)
