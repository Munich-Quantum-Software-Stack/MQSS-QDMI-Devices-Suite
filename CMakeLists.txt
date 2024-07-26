cmake_minimum_required(VERSION 3.22)

project(Backends
        VERSION 0.1
        DESCRIPTION "QDMI Backends")

include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "-g")

add_library(backend_ibm SHARED)
target_include_directories(backend_ibm PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

find_package(LLVM REQUIRED CONFIG)
target_include_directories(backend_ibm PUBLIC
    $<BUILD_INTERFACE:${LLVM_INCLUDE_DIRS}>
)

add_definitions(${LLVM_DEFINITIONS})

add_subdirectory(src)
add_subdirectory(tests)

# Install the backend_ibm library and export configuration
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

# Optionally install header files if they are in the include directory
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Uncomment and modify if you have a pkg-config file
# install(FILES ${CMAKE_BINARY_DIR}/mylib.pc
#     DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
