# ParityQC © 2025. See LICENSE.txt in /src/isv/parityqc/ for details.

enable_testing()
include(GoogleTest)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Creates qdmi_test_<prefix>_device_defs. Provided by QDMI project.
generate_device_defs_executable(${PARITYQC_QDMI_PREFIX})

add_executable(ParityOSDeviceTest ParityOSDeviceTest.cpp)

target_link_libraries(
  ParityOSDeviceTest PRIVATE gtest gtest_main qdmi::qdmi parityos_device
                             qdmi::project_warnings)

add_dependencies(ParityOSDeviceTest
                 qdmi_test_${PARITYQC_QDMI_PREFIX_LOWER}_device_defs)

gtest_discover_tests(ParityOSDeviceTest)

if(ENABLE_COVERAGE)
  target_compile_options(ParityOSDeviceTest INTERFACE --coverage -fprofile-arcs
                                                      -ftest-coverage -O0)
  target_link_libraries(ParityOSDeviceTest INTERFACE gcov --coverage)
endif()
