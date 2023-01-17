add_library(plugins Plugin.cpp)

target_link_libraries(plugins editor git lua Qt6::Core)

set_target_properties(plugins PROPERTIES AUTOMOC ON)
