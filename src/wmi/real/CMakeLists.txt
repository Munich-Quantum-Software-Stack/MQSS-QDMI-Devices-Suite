add_library(backend_wmi_real SHARED wmi_real.c)
target_link_libraries(backend_wmi_real PUBLIC qdmi cjson curl)
