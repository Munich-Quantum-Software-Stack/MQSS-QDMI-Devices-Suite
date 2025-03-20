enable_testing()
include(GoogleTest)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

generate_device_defs_executable(${QLM_QDMI_PREFIX})

add_executable(QLMBackendTest QLMBackendTest.cpp)

target_link_libraries(
  QLMBackendTest PRIVATE gtest gtest_main qdmi::qdmi qlm_device
                         qdmi::project_warnings)

add_dependencies(QLMBackendTest qdmi_test_qlm_device_defs)

gtest_discover_tests(QLMBackendTest)

if(ENABLE_COVERAGE)
    target_compile_options(QLMBackendTest INTERFACE --coverage -fprofile-arcs
                                          -ftest-coverage -O0)
    target_link_libraries(QLMBackendTest INTERFACE gcov --coverage)
endif()

