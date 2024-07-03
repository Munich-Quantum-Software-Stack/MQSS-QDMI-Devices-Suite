add_executable(IBMBackend IBMBackendTest.c)
target_link_libraries(IBMBackend backend_ibm)
add_test(IBMBackend IBMBackend)

add_executable(WMIBackend WMIBackendTest.c)
target_link_libraries(WMIBackend backend_wmi_real)
add_test(WMIBackend WMIBackend)