add_library(conf ConfFile.cpp Settings.cpp Setting.cpp RecentRepositories.cpp
                 RecentRepository.cpp)

target_link_libraries(conf lua Qt5::Core util)

target_compile_definitions(
  conf PRIVATE SCINTILLUA_LEXERS_DIR="${SCINTILLUA_LEXERS_DIR}")

if(UNIX)
  target_compile_definitions(
    conf PRIVATE CONF_DIR="${CMAKE_INSTALL_FULL_DATADIR}/gittyup/Resources"
                 L10N_DIR="${L10N_INSTALL_DIR}")
else()

  target_compile_definitions(conf PRIVATE CONF_DIR="" L10N_DIR="")
endif()
set_target_properties(conf PROPERTIES AUTOMOC ON)
