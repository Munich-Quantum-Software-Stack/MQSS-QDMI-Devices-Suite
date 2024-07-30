# Declare all external dependencies and make sure that they are available.

include(FetchContent)
set(FETCH_PACKAGES "")

set(QDMI_URL "https://github.com/Munich-Quantum-Software-Stack/QDMI.git" CACHE STRING "QDMI URL")
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
	FetchContent_Declare(qdmi GIT_REPOSITORY ${QDMI_URL} GIT_TAG develop)
	list(APPEND FETCH_PACKAGES qdmi)
else()
	find_package(qdmi QUIET)
	if(NOT qdmi_FOUND)
		FetchContent_Declare(qdmi GIT_REPOSITORY ${QDMI_URL} GIT_TAG develop)
		list(APPEND FETCH_PACKAGES qdmi)
	endif()
endif()

set(JANSSON_VERSION "2.14" CACHE STRING "Jansson version")
set(JANSSON_URL "https://github.com/akheron/jansson/releases/download/v${JANSSON_VERSION}/jansson-${JANSSON_VERSION}.tar.gz" CACHE STRING "Jansson URL")
set(JANSSON_BUILD_DOCS OFF CACHE INTERNAL "Do not build Jansson documentation")
set(JANSSON_WITHOUT_TESTS ON CACHE INTERNAL "Do not build Jansson tests")

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
	FetchContent_Declare(jansson URL ${JANSSON_URL} FIND_PACKAGE_ARGS ${JANSSON_VERSION})
	list(APPEND FETCH_PACKAGES jansson)
else()
	find_package(jansson ${JANSSON_VERSION} QUIET)
	if(NOT jansson_FOUND)
		FetchContent_Declare(jansson URL ${JANSSON_URL})
		list(APPEND FETCH_PACKAGES jansson)
	endif()
endif()

set(CJSON_VERSION "1.7.18" CACHE STRING "cJSON version")
set(CJSON_URL "https://github.com/DaveGamble/cJSON/archive/refs/tags/v${CJSON_VERSION}.tar.gz" CACHE STRING "cJSON URL")
set(ENABLE_CJSON_TEST OFF CACHE INTERNAL "Do not build cJSON tests")
set(ENABLE_CJSON_UNINSTALL OFF CACHE INTERNAL "Do not add cJSON uninstall target")
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
	FetchContent_Declare(cjson URL ${CJSON_URL} FIND_PACKAGE_ARGS ${CJSON_VERSION})
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

	# QDMI currently requires access to internal headers
	target_include_directories(qdmi PUBLIC $<BUILD_INTERFACE:${qdmi_SOURCE_DIR}/src>)

	# jansson does not set the include directory on the target
	target_include_directories(jansson PUBLIC $<BUILD_INTERFACE:${jansson_BINARY_DIR}/include>)

	# cJSON does not set the include directory on the target
	target_include_directories(cjson PUBLIC $<BUILD_INTERFACE:${cjson_SOURCE_DIR}>)
endif()
