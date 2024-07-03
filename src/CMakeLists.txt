set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake" ${CMAKE_MODULE_PATH})

find_package(qdmi REQUIRED)
find_package(jansson REQUIRED)
find_package(cJSON REQUIRED)
find_package(CURL REQUIRED)

target_include_directories(backend_ibm PUBLIC ${QDMI_SRC_DIRS}) # do better
target_include_directories(backend_ibm PUBLIC ${QDMI_INCLUDE_DIRS}) # do better
target_include_directories(backend_ibm PUBLIC ${CMAKE_BINARY_DIR}/_deps/qinfo-src/include) # do better

target_include_directories(backend_wmi_real PUBLIC ${QDMI_SRC_DIRS}) 
target_include_directories(backend_wmi_real PUBLIC ${QDMI_INCLUDE_DIRS}) 
target_include_directories(backend_wmi_real PUBLIC ${CMAKE_BINARY_DIR}/_deps/qinfo-src/include) 

target_include_directories(backend_wmi_sim PUBLIC ${QDMI_SRC_DIRS}) 
target_include_directories(backend_wmi_sim PUBLIC ${QDMI_INCLUDE_DIRS}) 
target_include_directories(backend_wmi_sim PUBLIC ${CMAKE_BINARY_DIR}/_deps/qinfo-src/include) 

target_link_libraries(backend_ibm PUBLIC qinfo qdmi LLVM jansson)
target_link_libraries(backend_wmi_real PUBLIC qinfo qdmi LLVM cjson curl)
target_link_libraries(backend_wmi_sim PUBLIC qinfo qdmi LLVM cjson curl)

target_sources(backend_ibm PRIVATE ibm.c)
target_sources(backend_wmi_real PRIVATE wmi_real.c)
target_sources(backend_wmi_sim PRIVATE wmi_sim.c)