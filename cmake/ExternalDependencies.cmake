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

include(FetchContent)
set(FETCH_PACKAGES "")

set(QDMI_VERSION
    "1.1.0"
    CACHE STRING "QDMI version")
set(QDMI_URL
    "https://github.com/kayaercument/QDMI.git"
    CACHE STRING "QDMI URL")

set(BUILD_QDMI_DOCS OFF)
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
  FetchContent_Declare(
    qdmi
    GIT_REPOSITORY https://github.com/kayaercument/QDMI.git
    GIT_TAG 152-env-prop)
  list(APPEND FETCH_PACKAGES qdmi)
else()
  find_package(qdmi ${QDMI_VERSION} QUIET)
  if(NOT qdmi_FOUND)
    FetchContent_Declare(qdmi URL ${QDMI_URL})
    list(APPEND FETCH_PACKAGES qdmi)
  endif()
endif()

# if(BUILD_DOCUMENTATION)
#   find_package(Doxygen 1.13.1 REQUIRED)

#   set(DOXYGEN_AWESOME_VERSION
#       1.12.0
#       CACHE STRING "Doxygen Awesome version")
#   set(DOXYGEN_AWESOME_REV
#       "af1d9030b3ffa7b483fa9997a7272fb12af6af4c"
#       CACHE STRING "Doxygen Awesome identifier (tag, branch or commit hash)")
#   if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
#     FetchContent_Declare(
#       doxygen-awesome-css
#       GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
#       GIT_TAG ${DOXYGEN_AWESOME_REV}
#       FIND_PACKAGE_ARGS ${DOXYGEN_AWESOME_VERSION})
#     list(APPEND FETCH_PACKAGES doxygen-awesome-css)
#   else()
#     find_package(doxygen-awesome-css ${DOXYGEN_AWESOME_VERSION} QUIET)
#     if(NOT doxygen-awesome-css_FOUND)
#       FetchContent_Declare(
#         doxygen-awesome-css
#         GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
#         GIT_TAG ${DOXYGEN_AWESOME_REV})
#       list(APPEND FETCH_PACKAGES doxygen-awesome-css)
#     endif()
#   endif()
# endif()

if(BUILD_BACKEND_TESTS)
  set(gtest_force_shared_crt
      ON
      CACHE BOOL "" FORCE)
  set(GTEST_VERSION
      1.16.0
      CACHE STRING "Google Test version")
  set(GTEST_URL
      https://github.com/google/googletest/archive/refs/tags/v${GTEST_VERSION}.tar.gz
  )
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    FetchContent_Declare(googletest URL ${GTEST_URL} FIND_PACKAGE_ARGS
                                        ${GTEST_VERSION} NAMES GTest)
    list(APPEND FETCH_PACKAGES googletest)
  else()
    find_package(googletest ${GTEST_VERSION} QUIET NAMES GTest)
    if(NOT googletest_FOUND)
      FetchContent_Declare(googletest URL ${GTEST_URL})
      list(APPEND FETCH_PACKAGES googletest)
    endif()
  endif()
endif()

if(FETCH_PACKAGES)
  FetchContent_MakeAvailable(${FETCH_PACKAGES})

endif()
