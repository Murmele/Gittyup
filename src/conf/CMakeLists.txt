add_library(conf ConfFile.cpp Settings.cpp RecentRepositories.cpp
                 RecentRepository.cpp)

target_link_libraries(conf lua Qt5::Core)

target_compile_definitions(
  conf PRIVATE SCINTILLUA_LEXERS_DIR="${SCINTILLUA_LEXERS_DIR}")

target_compile_definitions(conf PRIVATE CONF_DIR="${CMAKE_INSTALL_DATADIR}/Resources"
                                          L10N_DIR="${CMAKE_INSTALL_DATADIR}/Resources/l10n")

set_target_properties(conf PROPERTIES AUTOMOC ON)
