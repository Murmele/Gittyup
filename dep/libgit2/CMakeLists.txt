# Set some defaults.
set(BUILD_SHARED_LIBS
    OFF
    CACHE BOOL "" FORCE)
set(BUILD_TESTS
    OFF
    CACHE BOOL "" FORCE)
set(STATIC_CRT
    OFF
    CACHE BOOL "" FORCE)

# Enabled manually. Don't search for it.
set(USE_SSH
	ON
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

if(NOT USE_SYSTEM_LIBGIT2)
  add_subdirectory(libgit2)

  # Include the generated experimental.h
  target_include_directories(
    libgit2 INTERFACE ${PROJECT_BINARY_DIR}/dep/libgit2/libgit2/include/git2)
endif()
