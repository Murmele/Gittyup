add_library(util Path.cpp)

target_link_libraries(util Qt5::Core)

set_target_properties(util PROPERTIES AUTOMOC ON)
