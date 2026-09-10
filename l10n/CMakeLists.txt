option(UPDATE_TRANSLATIONS
       "Update translation (*.ts) files when source files change")

set(LANGUAGES
    de
    ru
    en
    es
    ja
    pt
    pt_BR
    sv
    zh_CN)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
file(GLOB_RECURSE SOURCE_FILES ${SRC_DIR}/*.h ${SRC_DIR}/*.cpp ${SRC_DIR}/*.mm)

set(SYSTEM_LANG_KEY "System")
set(SUPPORTED_LANGUAGES "{\"${SYSTEM_LANG_KEY}\", \"${SYSTEM_LANG_KEY}\"},")
foreach(LANGUAGE ${LANGUAGES})
  set(TS_FILES ${TS_FILES} gittyup_${LANGUAGE}.ts)
  set(SUPPORTED_LANGUAGES
      "${SUPPORTED_LANGUAGES} {\"${LANGUAGE}\", \"${LANGUAGE}\"},")
endforeach()

set(LANGUAGE_SOURCE_FILE "${CMAKE_CURRENT_BINARY_DIR}/languages.cpp")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/languages.cpp.inc"
               ${LANGUAGE_SOURCE_FILE} @ONLY)

if(UPDATE_TRANSLATIONS)
  # FIXME: Clean removes the .ts files.
  qt6_create_translation(QM_FILES ${SOURCE_FILES} ${TS_FILES})
else()
  qt6_add_translation(QM_FILES ${TS_FILES})
endif()

add_custom_target(translations DEPENDS ${QM_FILES})
add_dependencies(gittyup translations)

add_library(translation ${LANGUAGE_SOURCE_FILE})
target_include_directories(translation PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(translation Qt6::Core)

# install language files
#
# Standard Qt widget strings (buttons such as "Cancel"/"OK",
# "Show Details…"/"Hide Details…", file dialogs, etc.) are translated by
# qtbase_<lang>.qm, NOT by the application's gittyup_<lang>.ts. We must ship
# these files so they match the exact Qt that builds the app.
#
# Strategy (version-safe):
#   1. Copy qtbase_<lang>.qm from the translations directory of the Qt that is
#      actually used to compile this build (derived from Qt6::Core's location),
#      so the translation always matches the linked Qt version.
#   2. Only if that Qt does not ship its own translations (some distro packages
#      strip them), fall back to the copies bundled in this source directory.
# The resolved file is also copied into the build l10n directory so that
# non-installed (debug) builds can load it.

# Resolve the translations directory of the Qt we are building against.
set(QT_TRANS_DIR "")
get_target_property(_qt_core_loc Qt6::Core IMPORTED_LOCATION)
if(_qt_core_loc)
  get_filename_component(_qt_bin "${_qt_core_loc}" DIRECTORY)
  get_filename_component(_qt_prefix "${_qt_bin}" DIRECTORY)
  set(QT_TRANS_DIR "${_qt_prefix}/translations")
endif()
if(NOT IS_DIRECTORY "${QT_TRANS_DIR}" AND DEFINED QT_TRANSLATIONS_DIR)
  set(QT_TRANS_DIR "${QT_TRANSLATIONS_DIR}")
endif()

foreach(LANGUAGE ${LANGUAGES})
  set(QT_QM "")
  if(EXISTS "${QT_TRANS_DIR}/qtbase_${LANGUAGE}.qm")
    set(QT_QM "${QT_TRANS_DIR}/qtbase_${LANGUAGE}.qm")
  elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/qtbase_${LANGUAGE}.qm")
    set(QT_QM "${CMAKE_CURRENT_SOURCE_DIR}/qtbase_${LANGUAGE}.qm")
  endif()

  if(QT_QM)
    get_filename_component(QM_NAME ${QT_QM} NAME)
    # Available to non-installed (debug) builds that read from the build l10n dir.
    configure_file(${QT_QM}
                   "${CMAKE_BINARY_DIR}/l10n/${QM_NAME}"
                   COPYONLY)
    install(FILES ${QT_QM}
            DESTINATION ${L10N_INSTALL_DIR}
            COMPONENT ${GITTYUP_NAME})
  endif()

  if(APPLE)
    get_target_property(DIR gittyup RUNTIME_OUTPUT_DIRECTORY)
    add_custom_command(
      TARGET translations
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${DIR}/${RESOURCES_DIR}/${LANGUAGE}.lproj)
  endif()
endforeach()

foreach(QM_FILE ${QM_FILES})
  install(
    FILES ${QM_FILE}
    DESTINATION ${L10N_INSTALL_DIR}
    COMPONENT ${GITTYUP_NAME})
endforeach()
