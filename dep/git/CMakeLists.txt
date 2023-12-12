if(NOT USE_SYSTEM_GIT)
  set(PATH ${CMAKE_CURRENT_SOURCE_DIR}/git/contrib/credential)

  macro(add_helper NAME)
    set(TARGET git-credential-${NAME})
    add_executable(${TARGET} ${PATH}/${NAME}/${TARGET}.c)
    set_target_properties(
      ${TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                           $<TARGET_FILE_DIR:gittyup>/credential-helpers)

    if(${ARGC} GREATER 1)
      target_link_libraries(${TARGET} ${ARGV1})
    endif()

    if(NOT APPLE)
      install(
        TARGETS ${TARGET}
        DESTINATION ${CMAKE_INSTALL_BINDIR}/credential-helpers
        COMPONENT ${GITTYUP_NAME})
    endif()
  endmacro()

  if(APPLE)
    add_helper(osxkeychain "-framework Security")
  elseif(WIN32)
    add_helper(wincred)
  else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBSECRET IMPORTED_TARGET libsecret-1)
    if(LIBSECRET_FOUND)
      add_helper(libsecret PkgConfig::LIBSECRET)
    endif()

    pkg_check_modules(GNOME_KEYRING IMPORTED_TARGET gnome-keyring-1)
    if(GNOME_KEYRING_FOUND)
      add_helper(gnome-keyring PkgConfig::GNOME_KEYRING)
    endif()
  endif()
endif()
