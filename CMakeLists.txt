cmake_minimum_required(VERSION 3.22)

project(Backends
        VERSION 0.1
        DESCRIPTION "QDMI Backends")

include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "-g")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

find_package(qdmi REQUIRED)
find_package(qinfo REQUIRED)
find_package(cjson REQUIRED)

add_subdirectory(src)
add_subdirectory(tests)

