find_package(PkgConfig)

add_subdirectory(openssl)
add_subdirectory(libssh2)
add_subdirectory(libgit2)

add_subdirectory(cmark)
add_subdirectory(lua)
if(NOT LUA_FOUND)
	add_subdirectory(lpeg)
endif()
add_subdirectory(scintilla)

add_subdirectory(git)

add_subdirectory(hunspell)
