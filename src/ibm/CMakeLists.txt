set(IBM_INCLUDE_DIR ${BACKENDS_INCLUDE_BUILD_DIR}/ibm)

add_library(backend_ibm SHARED
            ibm.c
            ${IBM_INCLUDE_DIR}/ibm.h)
target_include_directories(backend_ibm PUBLIC ${IBM_INCLUDE_DIR})
target_link_libraries(backend_ibm PUBLIC qdmi jansson)
