# ParityQC © 2025. See LICENSE.txt in /src/isv/parityqc/ for details.

find_package(Python3 3.10...3.13 REQUIRED COMPONENTS Interpreter Development)

add_library(parityos_device SHARED parityos.cpp)

target_link_libraries(parityos_device PUBLIC qdmi::qdmi qdmi::project_warnings
                                             ${Python3_LIBRARIES})
generate_prefixed_qdmi_headers(${PARITYQC_QDMI_PREFIX})
target_include_directories(
  parityos_device PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include
                         ${Python3_INCLUDE_DIRS})
add_library(qdmi::parityos_device ALIAS parityos_device)

if(ENABLE_COVERAGE)
  target_compile_options(parityos_device INTERFACE --coverage -fprofile-arcs
                                                   -ftest-coverage -O0)
  target_link_libraries(parityos_device INTERFACE gcov --coverage)
endif()
