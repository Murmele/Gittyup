if(USE_SYSTEM_CMARK)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(CMARK libcmark)
    add_executable(cmark_exe IMPORTED GLOBAL)
    set_property(TARGET cmark_exe PROPERTY IMPORTED_LOCATION cmark)
  endif()

  set(LIBCMARK_LIBRARIES
      cmark
      CACHE INTERNAL "libcmark implementation" FORCE)

else()
  set(CMARK_TESTS
      OFF
      CACHE BOOL "" FORCE)
  set(CMARK_SHARED
      OFF
      CACHE BOOL "" FORCE)

  add_subdirectory(cmark)
  target_include_directories(
    cmark_static
    INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cmark/src>
              $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/cmark/src>)
  set(LIBCMARK_LIBRARIES
      cmark_static
      CACHE INTERNAL "libcmark implementation" FORCE)
endif()
