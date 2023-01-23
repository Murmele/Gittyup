add_library(conf ConfFile.cpp Settings.cpp Setting.cpp RecentRepositories.cpp
                 RecentRepository.cpp)

target_link_libraries(conf lua Qt5::Core util)

target_compile_definitions(
  conf PRIVATE SCINTILLUA_LEXERS_DIR="${SCINTILLUA_LEXERS_DIR}")

target_compile_definitions(
    conf PRIVATE CONF_DIR="${RESOURCES_DIR}"
                 L10N_DIR="${L10N_INSTALL_DIR}")
set_target_properties(conf PROPERTIES AUTOMOC ON)
