add_library(editor TextEditor.cpp)

target_link_libraries(editor lexilla scintilla scintillua ui)

set_target_properties(editor PROPERTIES AUTOMOC ON)
