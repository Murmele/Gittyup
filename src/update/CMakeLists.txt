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
  Qt6::Network
  Qt6::Widgets
  ${UPDATER_IMPL_LIBS})

set_target_properties(update PROPERTIES AUTOMOC ON)

add_executable(gittyup-relauncher relauncher.cpp)
target_link_libraries(gittyup-relauncher Qt6::Core)

set_target_properties(gittyup-relauncher PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                    $<TARGET_FILE_DIR:gittyup>)

if(APPLE)
  set_target_properties(
    gittyup-relauncher PROPERTIES INSTALL_RPATH
                                  "/usr/local/qt;@loader_path/../Frameworks")
elseif(UNIX)
  set_target_properties(gittyup-relauncher PROPERTIES INSTALL_RPATH "$ORIGIN")

  install(
    TARGETS gittyup-relauncher
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT ${GITTYUP_NAME})
endif()
