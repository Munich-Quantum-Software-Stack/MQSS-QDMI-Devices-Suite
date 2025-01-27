find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

add_library(backend_qaptiva_iqm SHARED qaptiva.c)
target_link_libraries(backend_qaptiva_iqm PUBLIC qdmi cjson ${Python3_LIBRARIES})
target_include_directories(backend_qaptiva_iqm PUBLIC ${Python3_INCLUDE_DIRS})