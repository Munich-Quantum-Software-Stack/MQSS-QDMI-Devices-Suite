# set required cmake version
cmake_minimum_required(VERSION 3.19...3.30)

project(
  Backends
  VERSION 0.1
  DESCRIPTION "QDMI Backends"
  LANGUAGES C)

# Generate compile_commands.json to make it easier to work with clang based
# tools
set(CMAKE_EXPORT_COMPILE_COMMANDS
    ON
    CACHE BOOL "Export compile commands" FORCE)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# set the include directory for the build tree
set(BACKENDS_INCLUDE_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Declare all external dependencies and make sure that they are available.
include(cmake/ExternalDependencies.cmake)

option(BUILD_BACKEND_QLM "Build QLM Backend" ON)

option(BUILD_BACKEND_DCDB "Build DCDB Backend" ON)

option(BUILD_DOCUMENTATION "Also build documentation" ON)

option(BUILD_BACKEND_TESTS "Also build tests for the backends" ON)

option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

# Require C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_EXTENSIONS False)

if(BUILD_BACKEND_QLM)
  set(QLM_QDMI_PREFIX "QLM")
endif()

if(BUILD_BACKEND_DCDB)
  set(DCDB_QDMI_PREFIX "DCDB")
endif()

if(ENABLE_COVERAGE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -O0 -g")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -O0 -g")
endif()

include(cmake/ExternalDependencies.cmake)

add_subdirectory(src)

if(BUILD_BACKEND_TESTS)
  add_subdirectory(tests)
endif()

if(BUILD_DOCUMENTATION)
  add_subdirectory(docs)
endif()
