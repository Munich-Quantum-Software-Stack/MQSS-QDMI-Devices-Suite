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

# The MQV device implements the QDMI 1.4 Device Interface, which MQT Core 4.1
# requires; the other modules of this repository are still on QDMI 1.3.
fetch_qdmi("Munich-Quantum-Software-Stack" "v1.4.0")

set(MQV_QDMI_DEVICE_ID
    "mqv.default"
    CACHE STRING "Stable identifier of the MQV QDMI device")
set(MQV_DEVICE_VERSION "0.1.0")
set(MQV_TARGET_NAME "mqv_device")

generate_prefixed_qdmi_headers(${MQV_QDMI_PREFIX})

add_library(${MQV_TARGET_NAME} SHARED mqv_device.cpp session.cpp)
add_library(qdmi::mqv_device ALIAS ${MQV_TARGET_NAME})
target_compile_features(${MQV_TARGET_NAME} PRIVATE cxx_std_20)
get_target_property(qdmi_library_VERSION qdmi::qdmi VERSION)
target_compile_definitions(
  ${MQV_TARGET_NAME}
  PRIVATE QDMI_VERSION="${qdmi_library_VERSION}"
          ${MQV_QDMI_PREFIX}_QDMI_DEVICE_ID="${MQV_QDMI_DEVICE_ID}"
          ${MQV_QDMI_PREFIX}_QDMI_DEVICE_VERSION="${MQV_DEVICE_VERSION}"
          ${MQV_QDMI_PREFIX}_QDMI_device_EXPORTS)
set_target_properties(
  ${MQV_TARGET_NAME}
  PROPERTIES C_VISIBILITY_PRESET hidden
             CXX_VISIBILITY_PRESET hidden
             VISIBILITY_INLINES_HIDDEN 1)
configure_qdmi_exports(TARGET ${MQV_TARGET_NAME} INTERFACE device PREFIX
                       ${MQV_QDMI_PREFIX})
configure_qdmi_device_target(TARGET ${MQV_TARGET_NAME} ID ${MQV_QDMI_DEVICE_ID}
                             PREFIX ${MQV_QDMI_PREFIX})
target_include_directories(
  ${MQV_TARGET_NAME}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

find_package(CURL REQUIRED)
target_link_libraries(
  ${MQV_TARGET_NAME}
  PUBLIC qdmi::qdmi
  PRIVATE qdmi::qdmi_project_warnings mqss_client CURL::libcurl)

if(APPLE)
  set_target_properties(${MQV_TARGET_NAME} PROPERTIES INSTALL_RPATH
                                                      "@loader_path")
else()
  set_target_properties(${MQV_TARGET_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()

# Manifest for MQT Core's built-in QDMI driver, next to the library. It lists
# the generic device mqv.default (no session block: base URL, token and resource
# come from the session or the environment) and one device ID mqv.<lowercase
# resource> per MQP-API resource in MQV_QDMI_MANIFEST_RESOURCES, whose session
# block only sets custom1 (the resource); base URL and token still come from the
# session or the environment. A site may set MQV_QDMI_MANIFEST_BASEURL to also
# write a base-url into each per-resource entry.
set(MQV_QDMI_MANIFEST_BASEURL
    ""
    CACHE STRING "Optional MQP-API base URL of the per-resource device IDs")
set(MQV_QDMI_MANIFEST_RESOURCES
    "AQT20;MAQCS;MUNIQC-Atoms20;Q20;Q5;QExa20;QLM;WMI3"
    CACHE STRING "MQP-API resources that get their own device ID")
set(_mqv_entry
    "      \"library\": \"$<TARGET_FILE_NAME:${MQV_TARGET_NAME}>\",\n      \"prefix\": \"${MQV_QDMI_PREFIX}\",\n      \"enabled\": true"
)
set(_mqv_devices
    "    {\n      \"id\": \"${MQV_QDMI_DEVICE_ID}\",\n${_mqv_entry}\n    }")
foreach(_mqv_resource IN LISTS MQV_QDMI_MANIFEST_RESOURCES)
  string(TOLOWER "${_mqv_resource}" _mqv_id)
  set(_mqv_session "        \"custom1\": \"${_mqv_resource}\"")
  if(MQV_QDMI_MANIFEST_BASEURL)
    set(_mqv_session
        "        \"base-url\": \"${MQV_QDMI_MANIFEST_BASEURL}\",\n${_mqv_session}"
    )
  endif()
  string(
    APPEND
    _mqv_devices
    ",\n    {\n      \"id\": \"mqv.${_mqv_id}\",\n${_mqv_entry},\n      \"session\": {\n${_mqv_session}\n      }\n    }"
  )
endforeach()
file(
  GENERATE
  OUTPUT "$<TARGET_FILE_DIR:${MQV_TARGET_NAME}>/mqv-qdmi-device.qdmi.json"
  CONTENT
    "{\n  \"schema-version\": 1,\n  \"qdmi\": {\n    \"devices\": [\n${_mqv_devices}\n    ]\n  }\n}\n"
)

option(MQV_QDMI_INSTALL "Install the MQV device library and its manifest" OFF)
option(MQV_QDMI_BUNDLE_RABBITMQ
       "Install librabbitmq next to the device (for systems without it)" OFF)
if(MQV_QDMI_INSTALL)
  include(GNUInstallDirs)
  install(TARGETS ${MQV_TARGET_NAME}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(
    FILES "$<TARGET_FILE_DIR:${MQV_TARGET_NAME}>/mqv-qdmi-device.qdmi.json"
    DESTINATION ${CMAKE_INSTALL_LIBDIR})
  if(MQV_QDMI_BUNDLE_RABBITMQ)
    # Install the real file under the runtime name the device links against (its
    # SONAME, e.g. librabbitmq.so.4 or librabbitmq.4.dylib).
    file(REAL_PATH "${MQV_RABBITMQ_LIBRARY}" _mqv_rabbitmq_real)
    file(GLOB _mqv_rabbitmq_runtime
         "${MQV_RABBITMQ_LIBRARY_DIR}/librabbitmq.so.[0-9]"
         "${MQV_RABBITMQ_LIBRARY_DIR}/librabbitmq.[0-9].dylib")
    list(LENGTH _mqv_rabbitmq_runtime _mqv_rabbitmq_count)
    if(NOT _mqv_rabbitmq_count EQUAL 1)
      message(
        FATAL_ERROR "Cannot determine the runtime name of rabbitmq-c in "
                    "${MQV_RABBITMQ_LIBRARY_DIR}: ${_mqv_rabbitmq_runtime}")
    endif()
    get_filename_component(_mqv_rabbitmq_name "${_mqv_rabbitmq_runtime}" NAME)
    install(
      FILES "${_mqv_rabbitmq_real}"
      DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RENAME "${_mqv_rabbitmq_name}")
  endif()
endif()
