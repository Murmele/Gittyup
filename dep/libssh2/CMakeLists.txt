if(USE_SYSTEM_LIBSSH2)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBSSH2 REQUIRED libssh2)
  endif()

else()
  set(BUILD_EXAMPLES
      OFF
      CACHE BOOL "" FORCE)
  set(BUILD_TESTING
      OFF
      CACHE BOOL "" FORCE)

  # Gittyup sets BUILD_SHARED_LIBS OFF at the top level; Qt may also set this
  # cache variable. Bundled libssh2 still needs a Windows DLL + import libs for
  # pack/CMakeLists.txt and the ssh2.lib/libssh2.lib copy in libssh2/src.
  if(WIN32)
    set(BUILD_STATIC_LIBS
        ON
        CACHE BOOL "Build libssh2 static library" FORCE)
    set(BUILD_SHARED_LIBS
        ON
        CACHE BOOL "Build libssh2 shared library" FORCE)
  endif()

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
