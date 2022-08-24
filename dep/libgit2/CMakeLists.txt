# Set some defaults.
set(BUILD_SHARED_LIBS
    OFF
    CACHE BOOL "" FORCE)
set(BUILD_CLAR
    OFF
    CACHE BOOL "" FORCE)
set(STATIC_CRT
    OFF
    CACHE BOOL "" FORCE)

# Enabled manually. Don't search for it.
set(USE_SSH
    OFF
    CACHE BOOL "" FORCE)

if(WIN32)
  # Disable search for OpenSSL.
  set(USE_OPENSSL
      OFF
      CACHE BOOL "" FORCE)

  # Set variables manually.
  set(OPENSSL_FOUND TRUE)
  set(OPENSSL_INCLUDE_DIR ${OPENSSL_DIR}/inc32)
  set(OPENSSL_LIBRARIES ${OPENSSL_DIR}/out32dll/libeay32.lib
                        ${OPENSSL_DIR}/out32dll/ssleay32.lib)

  # Disable nanosecond precision.
  set(USE_NSEC
      OFF
      CACHE BOOL "" FORCE)
endif()

add_subdirectory(libgit2)
# otherwise the zlib is not available in the src/git
set(LIBGIT2_DEPENDENCY_OBJECTS ${LIBGIT2_DEPENDENCY_OBJECTS} PARENT_SCOPE)
set(LIBGIT2_SYSTEM_LIBS ${LIBGIT2_SYSTEM_LIBS} PARENT_SCOPE)
set(LIBGIT2_INCLUDES ${LIBGIT2_INCLUDES} PARENT_SCOPE)
