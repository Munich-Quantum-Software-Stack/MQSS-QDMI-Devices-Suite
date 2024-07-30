add_library(backend_ibm SHARED ibm.c)
target_link_libraries(backend_ibm PUBLIC qdmi jansson)
