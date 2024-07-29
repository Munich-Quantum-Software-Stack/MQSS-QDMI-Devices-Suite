add_executable(WMIBackend WMIBackendTest.c)
target_link_libraries(WMIBackend backend_wmi_sim)
add_test(WMIBackend WMIBackend)

