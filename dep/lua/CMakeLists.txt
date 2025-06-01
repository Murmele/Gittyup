find_package(Lua)
if(LUA_FOUND)
  add_library(lua INTERFACE)
  target_include_directories(lua INTERFACE ${LUA_INCLUDE_DIR})
  target_link_libraries(lua INTERFACE ${LUA_LIBRARIES})
elseif(NOT USE_SYSTEM_LUA)
  set(LUA_DIR lua)

  add_library(
    lua

    ${LUA_DIR}/lapi.c      ${LUA_DIR}/lcode.c     ${LUA_DIR}/ldblib.c  ${LUA_DIR}/ldump.c  ${LUA_DIR}/linit.c   ${LUA_DIR}/lmathlib.c  ${LUA_DIR}/lobject.c   ${LUA_DIR}/lparser.c  ${LUA_DIR}/lstrlib.c  ${LUA_DIR}/ltests.c  ${LUA_DIR}/lundump.c   ${LUA_DIR}/lzio.c
    ${LUA_DIR}/lauxlib.c   ${LUA_DIR}/lcorolib.c  ${LUA_DIR}/ldebug.c  ${LUA_DIR}/lfunc.c  ${LUA_DIR}/liolib.c  ${LUA_DIR}/lmem.c      ${LUA_DIR}/lopcodes.c  ${LUA_DIR}/lstate.c   ${LUA_DIR}/ltable.c   ${LUA_DIR}/ltm.c     ${LUA_DIR}/lutf8lib.c  ${LUA_DIR}/onelua.c
    ${LUA_DIR}/lbaselib.c  ${LUA_DIR}/lctype.c    ${LUA_DIR}/ldo.c     ${LUA_DIR}/lgc.c    ${LUA_DIR}/llex.c    ${LUA_DIR}/loadlib.c   ${LUA_DIR}/loslib.c    ${LUA_DIR}/lstring.c  ${LUA_DIR}/ltablib.c  ${LUA_DIR}/lua.c     ${LUA_DIR}/lvm.c)

  target_include_directories(
    lua INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/${LUA_DIR})

  set_property(TARGET lua PROPERTY C_STANDARD 99)
endif()
