set(IBM_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/ibm)

find_package(jansson REQUIRED)

add_library(backend_ibm SHARED)
target_sources(backend_ibm PRIVATE ibm.c)
target_include_directories(backend_ibm PUBLIC ${IBM_INCLUDE_DIR} ${QDMI_SRC_DIRS}) 
target_link_libraries(backend_ibm PUBLIC qdmi jansson)

