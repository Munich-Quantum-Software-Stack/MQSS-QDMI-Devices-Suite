# This module finds the Jansson library
# Once done, this will define
#
#  JANSSON_FOUND - System has Jansson
#  JANSSON_INCLUDE_DIRS - The Jansson include directories
#  JANSSON_LIBRARIES - The libraries needed to use Jansson

# Find the Jansson library
find_path(JANSSON_INCLUDE_DIR jansson.h
    PATH_SUFFIXES jansson
    PATHS ${CMAKE_INSTALL_PREFIX}/include /usr/include /usr/local/include)

find_library(JANSSON_LIBRARY NAMES jansson
    PATH_SUFFIXES lib64 lib
    PATHS ${CMAKE_INSTALL_PREFIX}/lib /usr/lib /usr/local/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(jansson
    FOUND_VAR JANSSON_FOUND
    REQUIRED_VARS JANSSON_INCLUDE_DIR JANSSON_LIBRARY)

# Provide a hint to the user if the library is not found
if(NOT JANSSON_FOUND)
    message(STATUS "Jansson library not found")
else()
    set(JANSSON_LIBRARIES ${JANSSON_LIBRARY})
    set(JANSSON_INCLUDE_DIRS ${JANSSON_INCLUDE_DIR})
    message(STATUS "Jansson library found: ${JANSSON_LIBRARY}")
endif()

# Report results
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(jansson REQUIRED_VARS JANSSON_LIBRARY JANSSON_INCLUDE_DIR)
mark_as_advanced(JANSSON_LIBRARY JANSSON_INCLUDE_DIR)