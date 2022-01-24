pkg_check_modules(HUNSPELL hunspell)

if(HUNSPELL_FOUND)
  add_library(hunspell INTERFACE)
  #target_compile_definitions(hunspell PUBLIC HUNSPELL_STATIC)
  target_include_directories(hunspell INTERFACE ${HUNSPELL_INCLUDE_DIRS})
  target_link_libraries(hunspell INTERFACE ${HUNSPELL_LINK_LIBRARIES})
else()
  set(HUNSPELL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/hunspell)

  add_library(hunspell
    ${HUNSPELL_DIR}/src/hunspell/affentry.cxx
    ${HUNSPELL_DIR}/src/hunspell/affixmgr.cxx
    ${HUNSPELL_DIR}/src/hunspell/csutil.cxx
    ${HUNSPELL_DIR}/src/hunspell/filemgr.cxx
    ${HUNSPELL_DIR}/src/hunspell/hashmgr.cxx
    ${HUNSPELL_DIR}/src/hunspell/hunspell.cxx
    ${HUNSPELL_DIR}/src/hunspell/hunzip.cxx
    ${HUNSPELL_DIR}/src/hunspell/phonet.cxx
    ${HUNSPELL_DIR}/src/hunspell/replist.cxx
    ${HUNSPELL_DIR}/src/hunspell/suggestmgr.cxx
  )

  target_compile_definitions(hunspell
    PUBLIC
    HUNSPELL_STATIC
  )

  target_include_directories(hunspell
    PUBLIC
    ${HUNSPELL_DIR}/src/hunspell
  )

  set_target_properties(hunspell PROPERTIES
    AUTOMOC ON
  )
endif()
