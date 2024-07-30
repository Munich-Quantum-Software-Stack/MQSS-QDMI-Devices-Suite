add_library(backend_wmi_sim SHARED wmi_sim.c)
target_link_libraries(backend_wmi_sim PUBLIC qdmi cjson curl)
