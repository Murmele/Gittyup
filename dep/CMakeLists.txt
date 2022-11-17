find_package(PkgConfig)

add_subdirectory(openssl)
add_subdirectory(libssh2)
add_subdirectory(libgit2)
# otherwise the zlib is not available in the src/git
set(LIBGIT2_DEPENDENCY_OBJECTS
    ${LIBGIT2_DEPENDENCY_OBJECTS}
    PARENT_SCOPE)
set(LIBGIT2_SYSTEM_LIBS
    ${LIBGIT2_SYSTEM_LIBS}
    PARENT_SCOPE)
set(LIBGIT2_INCLUDES
    ${LIBGIT2_INCLUDES}
    PARENT_SCOPE)

add_subdirectory(cmark)
add_subdirectory(lua)
if(NOT LUA_FOUND)
  add_subdirectory(lpeg)
endif()
add_subdirectory(scintilla)

add_subdirectory(git)

add_subdirectory(hunspell)
