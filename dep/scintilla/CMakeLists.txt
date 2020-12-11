set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}")
message(STATUS "${CMAKE_MODULE_PATH}") 
find_package(QScintilla)

if(QSCINTILLA_FOUND)
  add_library(scintilla INTERFACE)
  target_include_directories(hunspell INTERFACE ${QSCINTILLA_INCLUDE_DIR})
  target_link_libraries(hunspell INTERFACE ${QSCINTILLA_LIBRARY})
  set(SCINTILLUA_LEXERS_DIR ${SCINTILLA_DIR}/lexlua CACHE INTERNAL "")
else()
  set(SCINTILLA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/scintilla-3.21.0)

  add_library(scintilla
    ${SCINTILLA_DIR}/src/AutoComplete.cxx
    ${SCINTILLA_DIR}/src/CallTip.cxx
    ${SCINTILLA_DIR}/src/CaseConvert.cxx
      ${SCINTILLA_DIR}/src/CaseFolder.cxx
      ${SCINTILLA_DIR}/src/Catalogue.cxx
      ${SCINTILLA_DIR}/src/CellBuffer.cxx
    ${SCINTILLA_DIR}/src/CharClassify.cxx
    ${SCINTILLA_DIR}/src/ContractionState.cxx
    ${SCINTILLA_DIR}/src/Decoration.cxx
    ${SCINTILLA_DIR}/src/Document.cxx
    ${SCINTILLA_DIR}/src/Editor.cxx
    ${SCINTILLA_DIR}/src/EditModel.cxx
    ${SCINTILLA_DIR}/src/EditView.cxx
    ${SCINTILLA_DIR}/src/ExternalLexer.cxx
    ${SCINTILLA_DIR}/src/Indicator.cxx
    ${SCINTILLA_DIR}/src/KeyMap.cxx
    ${SCINTILLA_DIR}/src/LineMarker.cxx
    ${SCINTILLA_DIR}/src/MarginView.cxx
    ${SCINTILLA_DIR}/src/PerLine.cxx
    ${SCINTILLA_DIR}/src/PositionCache.cxx
    ${SCINTILLA_DIR}/src/RESearch.cxx
    ${SCINTILLA_DIR}/src/RunStyles.cxx
    ${SCINTILLA_DIR}/src/ScintillaBase.cxx
    ${SCINTILLA_DIR}/src/Selection.cxx
    ${SCINTILLA_DIR}/src/Style.cxx
    ${SCINTILLA_DIR}/src/UniConversion.cxx
    ${SCINTILLA_DIR}/src/UniqueString.cxx
    ${SCINTILLA_DIR}/src/ViewStyle.cxx
    ${SCINTILLA_DIR}/src/XPM.cxx
    ${SCINTILLA_DIR}/lexlib/Accessor.cxx
    ${SCINTILLA_DIR}/lexlib/CharacterCategory.cxx
    ${SCINTILLA_DIR}/lexlib/CharacterSet.cxx
    ${SCINTILLA_DIR}/lexlib/LexerBase.cxx
    ${SCINTILLA_DIR}/lexlib/LexerModule.cxx
    ${SCINTILLA_DIR}/lexlib/LexerSimple.cxx
    ${SCINTILLA_DIR}/lexlib/PropSetSimple.cxx
    ${SCINTILLA_DIR}/lexlib/StyleContext.cxx
    ${SCINTILLA_DIR}/lexlib/WordList.cxx
  )

  target_compile_definitions(scintilla
    PUBLIC
      SCINTILLA_QT
      SCI_LEXER
    PRIVATE
      SCI_EMPTYCATALOGUE
  )

  target_include_directories(scintilla
    PUBLIC
      ${SCINTILLA_DIR}/include
      ${SCINTILLA_DIR}/lexlib
      ${SCINTILLA_DIR}/src
  )

  target_link_libraries(scintilla
    Qt5::Widgets
  )

  set_target_properties(scintilla PROPERTIES
    AUTOMOC ON
  )

  set(SCINTILLUA_LEXERS_DIR ${SCINTILLA_DIR}/lexlua CACHE INTERNAL "")
endif()
