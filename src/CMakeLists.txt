set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake" ${CMAKE_MODULE_PATH})

find_package(qdmi REQUIRED)
find_package(jansson REQUIRED)


target_include_directories(backend_ibm PUBLIC ${QDMI_SRC_DIRS}) # do better
target_include_directories(backend_ibm PUBLIC ${QDMI_INCLUDE_DIRS}) # do better
target_include_directories(backend_ibm PUBLIC ${CMAKE_BINARY_DIR}/_deps/qinfo-src/include) # do better

target_link_libraries(backend_ibm PUBLIC qinfo qdmi LLVM jansson)


target_sources(backend_ibm PRIVATE ibm.c)

