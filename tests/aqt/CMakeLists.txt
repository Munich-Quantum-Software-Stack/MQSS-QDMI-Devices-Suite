find_package(LLVM REQUIRED CONFIG)

add_executable(aqtBackendTest AQTBackendTest.cpp)
target_link_libraries(aqtBackendTest PUBLIC LLVM backend_aqt qdmi)
target_include_directories(aqtBackendTest PUBLIC ${LLVM_INCLUDE_DIRS})
add_test(aqtBackendTest aqtBackendTest)
