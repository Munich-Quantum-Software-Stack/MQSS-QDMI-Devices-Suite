add_executable(IBMBackend IBMBackendTest.c)
target_link_libraries(IBMBackend PRIVATE backend_ibm)
add_test(IBMBackend IBMBackend)
