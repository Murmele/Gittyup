add_executable(
  gittyup-indexer
  main.cpp
  indexer.cpp
  diffgrabber.cpp
  map.cpp
  reduce.cpp
  resultwriter.cpp
  common.cpp
  lexerpool.cpp
  idstorage.cpp)
target_link_libraries(gittyup-indexer index)
target_compile_definitions(gittyup-indexer
                           PRIVATE GITTYUP_VERSION="${GITTYUP_VERSION}")

set_target_properties(gittyup-indexer PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                 $<TARGET_FILE_DIR:gittyup>)

if(APPLE)
  set_target_properties(
    gittyup-indexer PROPERTIES INSTALL_RPATH
                               "/usr/local/qt;@loader_path/../Frameworks")
else()
  if(WIN32)
    target_link_libraries(gittyup-indexer Dbghelp.lib)
  elseif(NOT FLATAPK)
    set_target_properties(gittyup-indexer PROPERTIES INSTALL_RPATH "$ORIGIN")
  endif()

  install(
    TARGETS gittyup-indexer
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT ${GITTYUP_NAME})
endif()
