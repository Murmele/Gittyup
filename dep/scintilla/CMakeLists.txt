include(FetchContent)
# Sandboxed builds (e.g. Flatpak) have no network access during the CMake
# configure step, so they pre-fetch scintilla into scintilla as an offline
# source instead. When present, use it directly instead of downloading.
set(SCINTILLA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/scintilla)
if(EXISTS ${SCINTILLA_DIR}/include/Scintilla.h)
  set(FETCHCONTENT_SOURCE_DIR_SCINTILLA ${SCINTILLA_DIR})
endif()
FetchContent_Declare(
  scintilla
  URL https://downloads.sourceforge.net/project/scintilla/scintilla/5.6.6/scintilla566.tgz
      DOWNLOAD_EXTRACT_TIMESTAMP
      TRUE)
FetchContent_MakeAvailable(scintilla)

set(LEXILLA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lexilla)
set(SCINTILLA_DIR ${scintilla_SOURCE_DIR})
set(SCINTILLUA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/scintillua)

add_library(
  lexilla
  ${LEXILLA_DIR}/src/Lexilla.cxx
  ${LEXILLA_DIR}/lexlib/Accessor.cxx
  ${LEXILLA_DIR}/lexlib/CharacterCategory.cxx
  ${LEXILLA_DIR}/lexlib/CharacterSet.cxx
  ${LEXILLA_DIR}/lexlib/DefaultLexer.cxx
  ${LEXILLA_DIR}/lexlib/LexerBase.cxx
  ${LEXILLA_DIR}/lexlib/LexerModule.cxx
  ${LEXILLA_DIR}/lexlib/LexerSimple.cxx
  ${LEXILLA_DIR}/lexlib/PropSetSimple.cxx
  ${LEXILLA_DIR}/lexlib/StyleContext.cxx
  ${LEXILLA_DIR}/lexlib/WordList.cxx)

target_include_directories(
  lexilla PUBLIC ${LEXILLA_DIR}/lexlib ${LEXILLA_DIR}/include
                 ${SCINTILLA_DIR}/include)

add_library(
  scintilla
  ${SCINTILLA_DIR}/qt/ScintillaEdit/ScintillaDocument.cpp
  ${SCINTILLA_DIR}/qt/ScintillaEdit/ScintillaEdit.cpp
  ${SCINTILLA_DIR}/qt/ScintillaEditBase/PlatQt.cpp
  ${SCINTILLA_DIR}/qt/ScintillaEditBase/ScintillaQt.cpp
  ${SCINTILLA_DIR}/qt/ScintillaEditBase/ScintillaEditBase.cpp
  ${SCINTILLA_DIR}/src/AutoComplete.cxx
  ${SCINTILLA_DIR}/src/CallTip.cxx
  ${SCINTILLA_DIR}/src/CaseConvert.cxx
  ${SCINTILLA_DIR}/src/CaseFolder.cxx
  ${SCINTILLA_DIR}/src/CellBuffer.cxx
  ${SCINTILLA_DIR}/src/ChangeHistory.cxx
  ${SCINTILLA_DIR}/src/CharacterCategoryMap.cxx
  ${SCINTILLA_DIR}/src/CharacterType.cxx
  ${SCINTILLA_DIR}/src/CharClassify.cxx
  ${SCINTILLA_DIR}/src/ContractionState.cxx
  ${SCINTILLA_DIR}/src/DBCS.cxx
  ${SCINTILLA_DIR}/src/Decoration.cxx
  ${SCINTILLA_DIR}/src/Document.cxx
  ${SCINTILLA_DIR}/src/Editor.cxx
  ${SCINTILLA_DIR}/src/EditModel.cxx
  ${SCINTILLA_DIR}/src/EditView.cxx
  ${SCINTILLA_DIR}/src/Geometry.cxx
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
  ${SCINTILLA_DIR}/src/UndoHistory.cxx
  ${SCINTILLA_DIR}/src/UniConversion.cxx
  ${SCINTILLA_DIR}/src/UniqueString.cxx
  ${SCINTILLA_DIR}/src/ViewStyle.cxx
  ${SCINTILLA_DIR}/src/XPM.cxx)

target_compile_definitions(
  scintilla
  PUBLIC SCINTILLA_QT SCI_LEXER
  PRIVATE SCI_EMPTYCATALOGUE MAKING_LIBRARY)

target_include_directories(
  scintilla PUBLIC ${SCINTILLA_DIR}/src ${SCINTILLA_DIR}/qt/ScintillaEdit
                   ${SCINTILLA_DIR}/qt/ScintillaEditBase)

target_link_libraries(scintilla Qt6::Widgets Qt6::Core5Compat lexilla)

set_target_properties(scintilla PROPERTIES AUTOMOC ON)

add_library(scintillua STATIC ${SCINTILLUA_DIR}/Scintillua.cxx)

target_include_directories(scintilla PUBLIC ${SCINTILLUA_DIR})

target_link_libraries(scintillua PRIVATE lexilla lua lpeg)

set(SCINTILLUA_LEXERS_DIR
    ${RESOURCES_DIR}/lexers
    CACHE INTERNAL "")
set(SRC_SCINTILLUA_LEXERS_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/scintillua/lexers
    CACHE INTERNAL "")
