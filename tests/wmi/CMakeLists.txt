add_executable(WMIBackend WMIBackendTest.c)
target_link_libraries(WMIBackend PRIVATE backend_wmi_sim)
add_test(WMIBackend WMIBackend)
