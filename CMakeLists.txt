cmake_minimum_required(VERSION 3.22)

project(Backends
        VERSION 0.1
        DESCRIPTION "QDMI Backends")

include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "-g")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

#add_library(backend_ibm SHARED)
#target_include_directories(backend_ibm PUBLIC include)

#add_library(backend_wmi_real SHARED)
#target_include_directories(backend_wmi_real PUBLIC include)

#add_library(backend_wmi_sim SHARED)
#target_include_directories(backend_wmi_sim PUBLIC include)

#find_package(LLVM REQUIRED CONFIG)
#include_directories(${LLVM_INCLUDE_DIRS})
#add_definitions(${LLVM_DEFINITIONS})

find_package(qdmi REQUIRED)
find_package(qinfo REQUIRED)
find_package(cjson REQUIRED)

#install(TARGETS backend_ibm DESTINATION lib)

add_subdirectory(src)
add_subdirectory(tests)

