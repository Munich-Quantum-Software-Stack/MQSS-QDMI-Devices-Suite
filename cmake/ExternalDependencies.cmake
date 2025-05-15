# Declare all external dependencies and make sure that they are available.

include(FetchContent)
set(FETCH_PACKAGES "")

set(QDMI_VERSION
    "1.1.0"
    CACHE STRING "QDMI version")
set(QDMI_URL
    "https://github.com/kayaercument/QDMI.git"
    CACHE STRING "QDMI URL")

set(BUILD_QDMI_DOCS OFF)
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
FetchContent_Declare(
    qdmi
    GIT_REPOSITORY  https://github.com/kayaercument/QDMI.git
    GIT_TAG 152-env-prop
)
  list(APPEND FETCH_PACKAGES qdmi)
else()
  find_package(qdmi ${QDMI_VERSION} QUIET)
  if(NOT qdmi_FOUND)
    FetchContent_Declare(qdmi URL ${QDMI_URL})
    list(APPEND FETCH_PACKAGES qdmi)
  endif()
endif()

if(BUILD_BACKEND_TESTS)
  set(gtest_force_shared_crt
      ON
      CACHE BOOL "" FORCE)
  set(GTEST_VERSION
      1.16.0
      CACHE STRING "Google Test version")
  set(GTEST_URL
      https://github.com/google/googletest/archive/refs/tags/v${GTEST_VERSION}.tar.gz
  )
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    FetchContent_Declare(googletest URL ${GTEST_URL} FIND_PACKAGE_ARGS
                                        ${GTEST_VERSION} NAMES GTest)
    list(APPEND FETCH_PACKAGES googletest)
  else()
    find_package(googletest ${GTEST_VERSION} QUIET NAMES GTest)
    if(NOT googletest_FOUND)
      FetchContent_Declare(googletest URL ${GTEST_URL})
      list(APPEND FETCH_PACKAGES googletest)
    endif()
  endif()
endif()

if(FETCH_PACKAGES)
  FetchContent_MakeAvailable(${FETCH_PACKAGES})
  
endif()
