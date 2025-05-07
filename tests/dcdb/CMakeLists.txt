enable_testing()
include(GoogleTest)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


generate_device_defs_executable("DCDB")

add_executable(DCDBBackendTest DCDBBackendTest.cpp)

target_link_libraries(
  DCDBBackendTest PRIVATE gtest gtest_main qdmi::qdmi qdmi::dcdb_device
                         qdmi::project_warnings)

gtest_discover_tests(DCDBBackendTest)

add_dependencies(DCDBBackendTest qdmi_test_dcdb_device_defs)

if(ENABLE_COVERAGE)
    target_compile_options(DCDBBackendTest INTERFACE --coverage -fprofile-arcs
                                          -ftest-coverage -O0)
    target_link_libraries(DCDBBackendTest INTERFACE gcov --coverage)
endif()
