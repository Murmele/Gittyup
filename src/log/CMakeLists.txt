add_library(loglib LogDelegate.cpp LogEntry.cpp LogModel.cpp LogView.cpp)

target_link_libraries(loglib Qt6::Widgets)

set_target_properties(loglib PROPERTIES AUTOMOC ON)
