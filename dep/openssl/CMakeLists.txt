if(NOT USE_SYSTEM_OPENSSL)
  if(EXISTS ${CMAKE_BINARY_DIR}/../openssl)
    set(OPENSSL_DIR
        ${CMAKE_BINARY_DIR}/../openssl
        CACHE PATH "")
  else()
    set(OPENSSL_DIR
        ${CMAKE_CURRENT_SOURCE_DIR}/openssl
        CACHE PATH "")
  endif()

  # This is a hint for the FindOpenSSL module.
  set(OPENSSL_ROOT_DIR
      ${OPENSSL_DIR}
      PARENT_SCOPE)
endif()
