include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_compile_options(-Werror=switch)
# Uncomment to compile with more warnings
#add_compile_options(-Wall)
# Uncomment to compile with even more warnings
#add_compile_options(-Wextra)
# TODO: currently there are too many unused parameters which overwhelms the
#       warning output with `-Wall`. So let's leave fixing these for later.
add_compile_options(-Wno-unused-parameter)

add_subdirectory(util)
add_subdirectory(cli)
add_subdirectory(conf)
add_subdirectory(cred)
add_subdirectory(dialogs)
add_subdirectory(editor)
add_subdirectory(git)
add_subdirectory(host)
add_subdirectory(index)
add_subdirectory(log)
add_subdirectory(plugins)
add_subdirectory(tools)
add_subdirectory(ui)
add_subdirectory(update)
add_subdirectory(watcher)

# Add executable last.
add_subdirectory(app)
