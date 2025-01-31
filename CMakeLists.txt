# set required cmake version
cmake_minimum_required(VERSION 3.19...3.30)

project(Backends
    VERSION 0.1
    DESCRIPTION "QDMI Backends"
    LANGUAGES C)

# Generate compile_commands.json to make it easier to work with clang based tools
set(CMAKE_EXPORT_COMPILE_COMMANDS
    ON
    CACHE BOOL "Export compile commands" FORCE)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# set the include directory for the build tree
set(BACKENDS_INCLUDE_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Declare all external dependencies and make sure that they are available.
include(cmake/ExternalDependencies.cmake)


option(BUILD_BACKEND_QLM "Build QLM Backend" TRUE)

option(BUILD_BACKEND_IQM "Build QLM Backend" FALSE)

option(BUILD_BACKEND_AQT "Build QLM Backend" FALSE)

option(BUILD_BACKEND_TESTS "Also build tests for the backends" TRUE)

option(ENABLE_COVERAGE "Enable coverage reporting" TRUE)

# Require C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_EXTENSIONS False)

if(BUILD_BACKEND_QLM)
    set(QLM_QDMI_PREFIX "QLM")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
if(ENABLE_COVERAGE)
endif()
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")

add_subdirectory(src)
add_subdirectory(tests)
