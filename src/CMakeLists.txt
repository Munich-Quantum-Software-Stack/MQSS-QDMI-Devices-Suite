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

install(
    TARGETS backend_ibm
    EXPORT backend_ibmConfig
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

export(
    TARGETS backend_ibm
    NAMESPACE backend_ibm::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/backend_ibmConfig.cmake"
)

install(
    EXPORT backend_ibmConfig
    FILE backend_ibmConfig.cmake
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/backend_ibm"
    NAMESPACE backend_ibm::
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

