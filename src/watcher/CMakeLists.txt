if(APPLE)
  set(WATCHER_IMPL_FILE RepositoryWatcher_mac.cpp)
  set(WATCHER_IMPL_LIBS "-framework CoreServices")
elseif(WIN32)
  set(WATCHER_IMPL_FILE RepositoryWatcher_win.cpp)
elseif(LINUX)
  set(WATCHER_IMPL_FILE RepositoryWatcher_linux.cpp)
else()
  set(WATCHER_IMPL_FILE RepositoryWatcher_qt.cpp)
endif()

add_library(watcher RepositoryWatcher.cpp ${WATCHER_IMPL_FILE})

target_include_directories(watcher PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(watcher git Qt5::Core ${WATCHER_IMPL_LIBS})

set_target_properties(watcher PROPERTIES AUTOMOC ON)
