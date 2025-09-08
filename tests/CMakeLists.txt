if(BUILD_BACKEND_QLM AND BUILD_BACKEND_TESTS)
  add_subdirectory(qlm)
endif()

if(BUILD_BACKEND_DCDB AND BUILD_BACKEND_TESTS)
  add_subdirectory(dcdb)
endif()

if(BUILD_BACKEND_LRZ AND BUILD_BACKEND_TESTS)
  add_subdirectory(lrz)
endif()
