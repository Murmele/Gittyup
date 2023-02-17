add_library(index GenericLexer.cpp Index.cpp IndexModel.cpp Lexer.cpp
                  LPegLexer.cpp Query.cpp)

target_link_libraries(
  index
  conf
  git
  lpeg
  lua
  Qt5::Core
  Qt5::Concurrent)

set_target_properties(index PROPERTIES AUTOMOC ON)

add_executable(lexer_test lexer_test.cpp)
target_link_libraries(lexer_test index)

add_executable(index_test index_test.cpp)
target_link_libraries(index_test index Qt5::Widgets)

add_executable(indexer indexer.cpp)
target_link_libraries(indexer index)
target_compile_definitions(indexer PRIVATE GITTYUP_VERSION="${GITTYUP_VERSION}")

set_target_properties(indexer PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                         $<TARGET_FILE_DIR:gittyup>)

if(APPLE)
  set_target_properties(
    indexer PROPERTIES INSTALL_RPATH "/usr/local/qt;@loader_path/../Frameworks")
else()
  if(WIN32)
    target_link_libraries(indexer Dbghelp.lib)
  elseif(NOT FLATAPK)
    set_target_properties(indexer PROPERTIES INSTALL_RPATH "$ORIGIN")
  endif()

  if(FLATPAK)
    # Install application.
    install(
      TARGETS indexer
      DESTINATION ./bin # otherwise the executable will not be found by flatpak
      COMPONENT ${GITTYUP_NAME})
  else()
    install(
      TARGETS indexer
      DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT ${GITTYUP_NAME})
  endif()
endif()
