add_library(util Path.cpp Debug.h Debug.cpp)

target_link_libraries(util Qt5::Core)
target_include_directories(util INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(util PROPERTIES AUTOMOC ON)
