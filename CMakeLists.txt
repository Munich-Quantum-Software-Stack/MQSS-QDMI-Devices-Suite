cmake_minimum_required(VERSION 3.22)

project(Backends
        VERSION 0.1
        DESCRIPTION "QDMI Backends")

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
SET(CMAKE_C_FLAGS "-g")

add_library(backend_ibm SHARED)
target_include_directories(backend_ibm PUBLIC include)

add_library(backend_wmi_real SHARED)
target_include_directories(backend_wmi_real PUBLIC include)

add_library(backend_wmi_sim SHARED)
target_include_directories(backend_wmi_sim PUBLIC include)

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

#install(TARGETS backend_ibm DESTINATION lib)

add_subdirectory(src)
add_subdirectory(tests)


#add_executable(IBMBackend tests/IBMBackendTest.c)
#target_link_libraries(IBMBackend backend_ibm3)