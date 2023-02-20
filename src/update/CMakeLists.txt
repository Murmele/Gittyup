if(APPLE)
  set(UPDATER_IMPL_FILE Updater_mac.mm)
  set(UPDATER_IMPL_LIBS "-framework AppKit" "-framework Foundation")
elseif(WIN32)
  set(UPDATER_IMPL_FILE Updater_win.cpp)
endif()

add_library(update DownloadDialog.cpp UpdateDialog.cpp Updater.cpp
                   UpToDateDialog.cpp ${UPDATER_IMPL_FILE})

target_link_libraries(
  update
  conf
  ${LIBCMARK_LIBRARIES}
  git
  Qt5::Network
  Qt5::Widgets
  ${UPDATER_IMPL_LIBS})

set_target_properties(update PROPERTIES AUTOMOC ON)

add_executable(relauncher relauncher.cpp)
target_link_libraries(relauncher Qt5::Core)

set_target_properties(relauncher PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                            $<TARGET_FILE_DIR:gittyup>)

if(APPLE)
  set_target_properties(
    relauncher PROPERTIES INSTALL_RPATH
                          "/usr/local/qt;@loader_path/../Frameworks")
elseif(UNIX)
  set_target_properties(relauncher PROPERTIES INSTALL_RPATH "$ORIGIN")

  install(
    TARGETS relauncher
    DESTINATION
      $<IF:$<PLATFORM_ID:Darwin>,${CONTENTS_DIR},${CMAKE_INSTALL_BINDIR}>
    COMPONENT ${GITTYUP_NAME})
endif()
