if(USE_SYSTEM_LIBSSH2)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBSSH2 libssh2)
  endif()

else()
  set(BUILD_EXAMPLES
      OFF
      CACHE BOOL "" FORCE)
  set(BUILD_TESTING
      OFF
      CACHE BOOL "" FORCE)

  add_subdirectory(libssh2)

  set(LIBSSH2_FOUND
      TRUE
      PARENT_SCOPE)
  set(LIBSSH2_LIBRARIES
      libssh2
      PARENT_SCOPE)
  set(LIBSSH2_INCLUDE_DIRS
      ${CMAKE_CURRENT_SOURCE_DIR}/libssh2/include
      PARENT_SCOPE)
endif()
