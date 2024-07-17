set(WMI_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/wmi/real)

add_library(backend_wmi_real SHARED)
target_sources(backend_wmi_real PRIVATE wmi_real.c)
target_include_directories(backend_wmi_real PUBLIC ${WMI_INCLUDE_DIR} ${QDMI_INCLUDE_DIRS} ${QDMI_SRC_DIRS} ${QINFO_INCLUDE_DIRS} ${CJON_INCLUDE_DIRS})
target_link_libraries(backend_wmi_real PUBLIC qdmi cjson curl)


#add_library(backend_wmi_sim SHARED)
#target_include_directories(backend_wmi_sim PUBLIC include)
