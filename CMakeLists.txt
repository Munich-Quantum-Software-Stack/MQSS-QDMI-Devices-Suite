cmake_minimum_required(VERSION 3.22)

project(Backends
        VERSION 0.1
        DESCRIPTION "QDMI Backends")

include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "-g")

add_library(backend_ibm SHARED)
target_include_directories(backend_ibm PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

find_package(LLVM REQUIRED CONFIG)
target_include_directories(backend_ibm PUBLIC
    $<BUILD_INTERFACE:${LLVM_INCLUDE_DIRS}>
)

add_definitions(${LLVM_DEFINITIONS})

add_subdirectory(src)
add_subdirectory(tests)

