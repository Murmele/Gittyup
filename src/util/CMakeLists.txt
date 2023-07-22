add_library(gittyupUtil Path.cpp Debug.h Debug.cpp)

target_link_libraries(gittyupUtil Qt5::Core)
target_include_directories(gittyupUtil INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(gittyupUtil PROPERTIES AUTOMOC ON)
