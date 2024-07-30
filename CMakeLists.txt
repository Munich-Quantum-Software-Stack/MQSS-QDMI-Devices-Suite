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

add_subdirectory(src)
add_subdirectory(tests)
