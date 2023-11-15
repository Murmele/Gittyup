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
  qt5_create_translation(QM_FILES ${SOURCE_FILES} ${TS_FILES})
else()
  qt5_add_translation(QM_FILES ${TS_FILES})
endif()

add_custom_target(translations DEPENDS ${QM_FILES})
add_dependencies(gittyup translations)

add_library(translation ${LANGUAGE_SOURCE_FILE})
target_include_directories(translation PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(translation Qt5::Core)

# install language files

foreach(LANGUAGE ${LANGUAGES})
  set(QT_QM_FILES ${QT_QM_FILES} ${QT_TRANSLATIONS_DIR}/qtbase_${LANGUAGE}.qm)

  if(APPLE)
    get_target_property(DIR gittyup RUNTIME_OUTPUT_DIRECTORY)
    add_custom_command(
      TARGET translations
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory
              ${DIR}/${RESOURCES_DIR}/${LANGUAGE}.lproj)
  endif()
endforeach()

foreach(QM_FILE ${QT_QM_FILES})
  if(EXISTS ${QM_FILE})
    install(
      FILES ${QM_FILE}
      DESTINATION ${L10N_INSTALL_DIR}
      COMPONENT ${GITTYUP_NAME})
  endif()
endforeach()

foreach(QM_FILE ${QM_FILES})
  install(
    FILES ${QM_FILE}
    DESTINATION ${L10N_INSTALL_DIR}
    COMPONENT ${GITTYUP_NAME})
endforeach()
