set(WMI_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/wmi/sim)

add_library(backend_wmi_sim SHARED)
target_sources(backend_wmi_sim PRIVATE wmi_sim.c)
target_include_directories(backend_wmi_sim PUBLIC ${WMI_INCLUDE_DIR} ${CJON_INCLUDE_DIRS} ${QDMI_SRC_DIRS})
target_link_libraries(backend_wmi_sim PUBLIC qdmi cjson curl)
