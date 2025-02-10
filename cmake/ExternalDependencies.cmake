# Declare all external dependencies and make sure that they are available.

include(FetchContent)
set(FETCH_PACKAGES "")

set(QDMI_VERSION
    "1.1.0"
    CACHE STRING "QDMI version")
set(QDMI_URL
    "https://github.com/Munich-Quantum-Software-Stack/QDMI/archive/refs/tags/v${QDMI_VERSION}.tar.gz"
    CACHE STRING "QDMI URL")
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
  FetchContent_Declare(qdmi URL ${QDMI_URL} FIND_PACKAGE_ARGS ${QDMI_VERSION})
  list(APPEND FETCH_PACKAGES qdmi)
else()
  find_package(qdmi ${QDMI_VERSION} QUIET)
  if(NOT qdmi_FOUND)
    FetchContent_Declare(qdmi URL ${QDMI_URL})
    list(APPEND FETCH_PACKAGES qdmi)
  endif()
endif()

set(CJSON_VERSION
    "1.7.18"
    CACHE STRING "cJSON version")
set(CJSON_URL
    "https://github.com/DaveGamble/cJSON/archive/refs/tags/v${CJSON_VERSION}.tar.gz"
    CACHE STRING "cJSON URL")
set(ENABLE_CJSON_TEST
    OFF
    CACHE INTERNAL "Do not build cJSON tests")
set(ENABLE_CJSON_UNINSTALL
    OFF
    CACHE INTERNAL "Do not add cJSON uninstall target")
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
  FetchContent_Declare(cjson URL ${CJSON_URL} FIND_PACKAGE_ARGS
                                 ${CJSON_VERSION})
  list(APPEND FETCH_PACKAGES cjson)
else()
  find_package(cjson ${CJSON_VERSION} QUIET)
  if(NOT cjson_FOUND)
    FetchContent_Declare(cjson URL ${CJSON_URL})
    list(APPEND FETCH_PACKAGES cjson)
  endif()
endif()

if(FETCH_PACKAGES)
  FetchContent_MakeAvailable(${FETCH_PACKAGES})

  target_include_directories(
    cjson PUBLIC $<BUILD_INTERFACE:${QDMI_INCLUDE_BUILD_DIR}>)
  # cJSON does not set the include directory on the target
  target_include_directories(cjson
                             PUBLIC $<BUILD_INTERFACE:${cjson_SOURCE_DIR}>)
endif()
