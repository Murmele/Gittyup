add_library(conf ConfFile.cpp Settings.cpp Setting.cpp RecentRepositories.cpp
                 RecentRepository.cpp)

target_link_libraries(conf lua Qt5::Core util translation)

# SRC_ definitions point to the source directly
target_compile_definitions(
  conf
  PRIVATE CONF_DIR="${RESOURCES_DIR}"
          SRC_CONF_DIR="${CMAKE_SOURCE_DIR}/conf"
          L10N_DIR="${L10N_INSTALL_DIR}"
          SRC_L10N_DIR="${CMAKE_BINARY_DIR}/l10n"
          SCINTILLUA_LEXERS_DIR="${SCINTILLUA_LEXERS_DIR}"
          SRC_SCINTILLUA_LEXERS_DIR="${SRC_SCINTILLUA_LEXERS_DIR}")
set_target_properties(conf PROPERTIES AUTOMOC ON)
