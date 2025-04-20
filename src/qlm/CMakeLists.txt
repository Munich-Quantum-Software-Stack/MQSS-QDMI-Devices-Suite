find_package(Python3 3.10...3.12 REQUIRED COMPONENTS Interpreter Development)

add_library(qlm_device SHARED qlm.c)

target_link_libraries(qlm_device PUBLIC qdmi::qdmi qdmi::project_warnings cjson
                                        ${Python3_LIBRARIES})
generate_prefixed_qdmi_headers(${QLM_QDMI_PREFIX})
target_include_directories(qlm_device PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include
                                             ${Python3_INCLUDE_DIRS})
add_library(qdmi::qlm_device ALIAS qlm_device)

if(ENABLE_COVERAGE)
    target_compile_options(qlm_device INTERFACE --coverage -fprofile-arcs
                                          -ftest-coverage -O0)
    target_link_libraries(qlm_device INTERFACE gcov --coverage)
endif()
