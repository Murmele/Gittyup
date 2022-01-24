find_package(Lua)
if(LUA_FOUND)
  add_library(lua INTERFACE)
  target_include_directories(lua INTERFACE ${LUA_INCLUDE_DIR})
  target_link_libraries(lua INTERFACE ${LUA_LIBRARIES})
else()
  set(LUA_DIR lua-5.4.0)

  add_library(lua
    ${LUA_DIR}/src/lapi.c
    ${LUA_DIR}/src/lauxlib.c
    ${LUA_DIR}/src/lbaselib.c
    ${LUA_DIR}/src/lcode.c
    ${LUA_DIR}/src/lcorolib.c
    ${LUA_DIR}/src/lctype.c
    ${LUA_DIR}/src/ldblib.c
    ${LUA_DIR}/src/ldebug.c
    ${LUA_DIR}/src/ldo.c
    ${LUA_DIR}/src/ldump.c
    ${LUA_DIR}/src/lfunc.c
    ${LUA_DIR}/src/lgc.c
    ${LUA_DIR}/src/linit.c
    ${LUA_DIR}/src/liolib.c
    ${LUA_DIR}/src/llex.c
    ${LUA_DIR}/src/lmathlib.c
    ${LUA_DIR}/src/lmem.c
    ${LUA_DIR}/src/loadlib.c
    ${LUA_DIR}/src/lobject.c
    ${LUA_DIR}/src/lopcodes.c
    ${LUA_DIR}/src/loslib.c
    ${LUA_DIR}/src/lparser.c
    ${LUA_DIR}/src/lstate.c
    ${LUA_DIR}/src/lstring.c
    ${LUA_DIR}/src/lstrlib.c
    ${LUA_DIR}/src/ltable.c
    ${LUA_DIR}/src/ltablib.c
    ${LUA_DIR}/src/ltm.c
    ${LUA_DIR}/src/lua.c
    ${LUA_DIR}/src/luac.c
    ${LUA_DIR}/src/lundump.c
    ${LUA_DIR}/src/lutf8lib.c
    ${LUA_DIR}/src/lvm.c
    ${LUA_DIR}/src/lzio.c
  )

  target_include_directories(lua INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/${LUA_DIR}/src
  )
endif()