set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake" ${CMAKE_MODULE_PATH})

find_package(qdmi REQUIRED)
find_package(jansson REQUIRED)

target_include_directories(backend_ibm PUBLIC
    $<BUILD_INTERFACE:${QDMI_SRC_DIRS}>
    $<BUILD_INTERFACE:${QDMI_INCLUDE_DIRS}>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/_deps/qinfo-src/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(backend_ibm PUBLIC qinfo qdmi LLVM jansson)

target_sources(backend_ibm PRIVATE ibm.c)
