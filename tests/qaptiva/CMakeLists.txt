enable_testing()
include(GoogleTest)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)



generate_device_defs_executable(${QLM_QDMI_PREFIX})

add_executable(QaptivaBackendTest QaptivaBackendTest.cpp)
target_compile_options(QaptivaBackendTest PRIVATE --coverage -fprofile-arcs -ftest-coverage -O0 -g)
target_link_libraries(QaptivaBackendTest PRIVATE --coverage gcov)

target_link_libraries(QaptivaBackendTest PRIVATE gtest gtest_main qdmi::qdmi qlm_device
  qdmi::project_warnings gcov)

add_dependencies(QaptivaBackendTest qdmi_test_qlm_device_defs)



gtest_discover_tests(
  QaptivaBackendTest)
