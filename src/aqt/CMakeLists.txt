find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

add_library(backend_aqt SHARED aqt.c)
target_link_libraries(backend_aqt PUBLIC qdmi cjson curl ${Python3_LIBRARIES})
target_include_directories(backend_aqt PUBLIC ${Python3_INCLUDE_DIRS})