enable_testing()
include(GoogleTest)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

generate_device_defs_executable(${QLM_QDMI_PREFIX})

add_executable(QLMBackendTest QLMBackendTest.cpp)
target_compile_options(QLMBackendTest PRIVATE --coverage -fprofile-arcs
                                              -ftest-coverage -O0 -g)
target_link_libraries(QLMBackendTest PRIVATE --coverage gcov)

target_link_libraries(
  QLMBackendTest PRIVATE gtest gtest_main qdmi::qdmi qlm_device
                         qdmi::project_warnings gcov)

add_dependencies(QLMBackendTest qdmi_test_qlm_device_defs)

gtest_discover_tests(QLMBackendTest)
