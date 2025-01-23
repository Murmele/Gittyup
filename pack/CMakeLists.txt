set(CONF_DIR ${CMAKE_SOURCE_DIR}/conf)
set(RSRC_DIR ${CMAKE_SOURCE_DIR}/rsrc)

set(MAC $<PLATFORM_ID:Darwin>)

# Install Qt plugins.
set(QT_PLUGINS QJpegPlugin)

if(APPLE)
  set(QT_PLUGINS ${QT_PLUGINS} QCocoaIntegrationPlugin QMacStylePlugin)
elseif(WIN32)
  set(QT_PLUGINS ${QT_PLUGINS} QWindowsIntegrationPlugin
                 QWindowsVistaStylePlugin)
else()
  set(QT_PLUGINS ${QT_PLUGINS} QXcbIntegrationPlugin
                 QComposePlatformInputContextPlugin)
endif()

if(WIN32)
  set(INSTALL_LIBDIR ${CMAKE_INSTALL_BINDIR})
elseif(NOT APPLE)
  set(INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
else()
  set(INSTALL_LIBDIR ${CONTENTS_DIR})
endif()

if(FLATPAK)
  qt_import_plugins(gittyup INCLUDE ${QT_PLUGINS})
elseif(NOT USE_SYSTEM_QT)
  foreach(QT_PLUGIN ${QT_PLUGINS})
    if(NOT TARGET Qt5::${QT_PLUGIN})
      if(UNIX OR APPLE)
        continue()
      endif()
    endif()

    get_target_property(PLUGIN Qt5::${QT_PLUGIN} LOCATION_${CMAKE_BUILD_TYPE})
    get_filename_component(FILE_NAME ${PLUGIN} NAME)
    get_filename_component(DIR ${PLUGIN} DIRECTORY)
    get_filename_component(DIR_NAME ${DIR} NAME)
    set(PLUGIN_PATH Plugins/${DIR_NAME}/${FILE_NAME})

    install(
      FILES ${PLUGIN}
      DESTINATION ${INSTALL_LIBDIR}/Plugins/${DIR_NAME}
      PERMISSIONS
        OWNER_READ
        OWNER_WRITE
        OWNER_EXECUTE
        GROUP_READ
        GROUP_EXECUTE
        WORLD_READ
        WORLD_EXECUTE
      COMPONENT ${GITTYUP_NAME})

    if(WIN32)
      # Copy into build dir.
      execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PLUGIN}
                              ${CMAKE_BINARY_DIR}/${PLUGIN_PATH})
    elseif(APPLE)
      # Sign plugin.
      if(CODESIGN_IDENTITY)
        install(
          CODE "execute_process(COMMAND
            codesign --timestamp --options runtime
            -s \"${CODESIGN_IDENTITY}\"
            \${CMAKE_INSTALL_PREFIX}/${CONTENTS_DIR}/${PLUGIN_PATH}
          )"
          COMPONENT ${GITTYUP_NAME})
      endif()
    else()
      # Delete runpath.
      install(CODE "execute_process(COMMAND chrpath --delete
          \${CMAKE_INSTALL_PREFIX}/${PLUGIN_PATH}
        )" COMPONENT ${GITTYUP_NAME})
    endif()
  endforeach()

  # Install Qt libraries.
  foreach(QT_MODULE ${QT_MODULES})
    get_target_property(QT_LIBRARY Qt5::${QT_MODULE}
                        LOCATION_${CMAKE_BUILD_TYPE})

    if(APPLE)
      get_filename_component(QT_FRAMEWORK ${QT_LIBRARY} DIRECTORY)
      set(DEST_DIR ${CONTENTS_DIR}/Frameworks)
      install(
        DIRECTORY ${QT_FRAMEWORK}
        DESTINATION ${DEST_DIR}
        COMPONENT ${GITTYUP_NAME}
        PATTERN "*.prl" EXCLUDE
        PATTERN "*_debug" EXCLUDE
        PATTERN "Headers" EXCLUDE
        PATTERN ".DS_Store" EXCLUDE)
    else()
      if(WIN32)
        get_filename_component(TARGET_NAME ${QT_LIBRARY} NAME)

        # Copy into build dir.
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                ${QT_LIBRARY} ${CMAKE_BINARY_DIR})
      else()
        # Strip minor and patch version extensions.
        get_filename_component(TARGET_NAME_WE ${QT_LIBRARY} NAME_WE)
        set(TARGET_NAME ${TARGET_NAME_WE}.so.5)
      endif()

      install(
        FILES ${QT_LIBRARY}
        DESTINATION ${INSTALL_LIBDIR}
        PERMISSIONS
          OWNER_READ
          OWNER_WRITE
          OWNER_EXECUTE
          GROUP_READ
          GROUP_EXECUTE
          WORLD_READ
          WORLD_EXECUTE
        COMPONENT ${GITTYUP_NAME}
        RENAME ${TARGET_NAME})

      if(NOT WIN32)
        # Delete runpath.
        install(CODE "execute_process(COMMAND chrpath --delete
            \${CMAKE_INSTALL_PREFIX}/${TARGET_NAME}
          )" COMPONENT ${GITTYUP_NAME})
      endif()
    endif()
  endforeach()
endif()

# Install XcbQpa library.
if(UNIX AND NOT APPLE)
  set(LIB_NAME libQt5XcbQpa)
  get_target_property(QT_CORE_LIBRARY Qt5::Core LOCATION)
  get_filename_component(LIB_PATH ${QT_CORE_LIBRARY} PATH)
  get_filename_component(LIB_EXT ${QT_CORE_LIBRARY} EXT)

  install(
    FILES "${LIB_PATH}/${LIB_NAME}${LIB_EXT}"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PERMISSIONS
      OWNER_READ
      OWNER_WRITE
      OWNER_EXECUTE
      GROUP_READ
      GROUP_EXECUTE
      WORLD_READ
      WORLD_EXECUTE
    COMPONENT ${GITTYUP_NAME}
    RENAME ${LIB_NAME}.so.5)
endif()

# Install SSL libraries.
if(NOT APPLE)
  if(NOT USE_SYSTEM_OPENSSL)
    if(WIN32)
      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SSL_LIB_SUFFIX "-3_4-x64.dll")
      else()
        set(SSL_LIB_SUFFIX "-3_4.dll")
      endif()
    else()
      set(SSL_LIB_SUFFIX ".so.3.4")
    endif()

    foreach(SSL_LIB_NAME ssl crypto)
      set(SSL_LIB "${OPENSSL_DIR}/lib${SSL_LIB_NAME}${SSL_LIB_SUFFIX}")

      # Copy into build dir.
      execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SSL_LIB}
                              ${CMAKE_BINARY_DIR})

      install(
        FILES ${SSL_LIB}
        DESTINATION ${INSTALL_LIBDIR}
        PERMISSIONS
          OWNER_READ
          OWNER_WRITE
          OWNER_EXECUTE
          GROUP_READ
          GROUP_EXECUTE
          WORLD_READ
          WORLD_EXECUTE
        COMPONENT ${GITTYUP_NAME}
      )
    endforeach()
  endif()

  if(WIN32 OR APPLE)
    # Install config files.
    set(QT_CONF ${CONF_DIR}/qt.conf)
    install(
      FILES ${QT_CONF}
      DESTINATION
        $<${MAC}:${RESOURCES_DIR}>$<$<PLATFORM_ID:Windows>:${INSTALL_LIBDIR}>
      COMPONENT ${GITTYUP_NAME})
  endif()
endif()

if(UNIX AND NOT APPLE)
  foreach(icon 16 32 64 128 256 512)
    install(
      FILES ${RSRC_DIR}/Gittyup.iconset/icon_${icon}x${icon}.png
      DESTINATION share/icons/hicolor/${icon}x${icon}/apps
      COMPONENT ${GITTYUP_NAME}
      RENAME ${GITTYUP_EXECUTABLE_NAME}.png)
  endforeach()
  install(
    FILES ${RSRC_DIR}/Gittyup.iconset/gittyup_logo.svg
    DESTINATION share/icons/hicolor/scalable/apps
    COMPONENT ${GITTYUP_NAME}
    RENAME ${GITTYUP_EXECUTABLE_NAME}.svg)
endif()

# Sign bundle on macOS.
if(APPLE AND CODESIGN_IDENTITY)
  install(
    CODE "execute_process(COMMAND
      codesign --deep --timestamp --options runtime
        -s \"${CODESIGN_IDENTITY}\"
        \${CMAKE_INSTALL_PREFIX}/${GITTYUP_EXECUTABLE_NAME}.app
    )"
    COMPONENT ${GITTYUP_NAME})
endif()

# Add package rule.
if(APPLE)
  set(CPACK_GENERATOR DragNDrop)
  set(CPACK_DMG_VOLUME_NAME ${GITTYUP_NAME})
  set(CPACK_DMG_DS_STORE ${RSRC_DIR}/DS_Store)
  set(CPACK_DMG_BACKGROUND_IMAGE ${RSRC_DIR}/dmg-background.png)
elseif(WIN32)
  set(CPACK_GENERATOR NSIS)
  set(CPACK_NSIS_CREATE_ICONS_EXTRA
      "CreateShortCut \
    \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${GITTYUP_NAME}.lnk\\\" \
    \\\"$INSTDIR\\\\${CMAKE_INSTALL_BINDIR}\\\\${GITTYUP_EXECUTABLE_NAME}.exe\\\""
  )
  set(CPACK_NSIS_EXECUTABLES_DIRECTORY ${CMAKE_INSTALL_BINDIR})
  set(CPACK_NSIS_MUI_FINISHPAGE_RUN ${GITTYUP_EXECUTABLE_NAME}.exe)

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    install(
      FILES ${RSRC_DIR}/vcredist_x64.exe
      DESTINATION .
      COMPONENT ${GITTYUP_NAME})
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
        "ExecWait '\\\"$INSTDIR\\\\vcredist_x64.exe\\\" /q /norestart'")
    set(PLATFORM "-win64")
  else()
    install(
      FILES ${RSRC_DIR}/vcredist_x86.exe
      DESTINATION .
      COMPONENT ${GITTYUP_NAME})
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
        "ExecWait '\\\"$INSTDIR\\\\vcredist_x86.exe\\\" /q /norestart'")
    set(PLATFORM "-win32")
  endif()
else()
  set(CPACK_GENERATOR TGZ)
endif()

if(DEV_BUILD)
  set(BUILD_TAG "-dev")
else()
  set(BUILD_TAG "")
endif()
set(PACKAGE_FILE_NAME
    "${GITTYUP_NAME}${PLATFORM}-${GITTYUP_VERSION}${BUILD_TAG}")

set(CPACK_MONOLITHIC_INSTALL true)
set(CPACK_INSTALL_CMAKE_PROJECTS ${CMAKE_BINARY_DIR} ${GITTYUP_NAME}
                                 ${GITTYUP_NAME} /)

# Add Context Menu Shortcuts
if(WIN32)
  set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
      "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}
    WriteRegStr HKCR 'Directory\\\\Background\\\\shell\\\\Gittyup' '' 'Open with Gittyup'
    WriteRegStr HKCR 'Directory\\\\Background\\\\shell\\\\Gittyup' 'Icon' '\\\"$INSTDIR\\\\${CMAKE_INSTALL_BINDIR}\\\\${GITTYUP_EXECUTABLE_NAME}.exe\\\"'
    WriteRegStr HKCR 'Directory\\\\Background\\\\shell\\\\Gittyup\\\\command' '' '\\\"$INSTDIR\\\\${CMAKE_INSTALL_BINDIR}\\\\${GITTYUP_EXECUTABLE_NAME}.exe\\\" \\\"%V\\\"'
    WriteRegStr HKCR 'Directory\\\\shell\\\\Gittyup' '' 'Open with Gittyup'
    WriteRegStr HKCR 'Directory\\\\shell\\\\Gittyup' 'Icon' '\\\"$INSTDIR\\\\${CMAKE_INSTALL_BINDIR}\\\\${GITTYUP_EXECUTABLE_NAME}.exe\\\"'
    WriteRegStr HKCR 'Directory\\\\shell\\\\Gittyup\\\\command' '' '\\\"$INSTDIR\\\\${CMAKE_INSTALL_BINDIR}\\\\${GITTYUP_EXECUTABLE_NAME}.exe\\\" \\\"%L\\\"'"
  )
  set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
      "${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS}
    DeleteRegKey HKCR 'Directory\\\\Background\\\\shell\\\\Gittyup'
    DeleteRegKey HKCR 'Directory\\\\shell\\\\Gittyup'")
endif(WIN32)

set(CPACK_PACKAGE_FILE_NAME ${PACKAGE_FILE_NAME})
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_BINARY_DIR}/pack)
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${GITTYUP_NAME})
set(CPACK_PACKAGE_VENDOR "Gittyup Contributors")
set(CPACK_PACKAGE_VERSION_MAJOR ${GITTYUP_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${GITTYUP_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${GITTYUP_VERSION_PATCH})

if(WIN32)
  set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE.md)
else()
  install(
    FILES ${CMAKE_SOURCE_DIR}/LICENSE.md
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/Gittyup
    RENAME LICENSE)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
include(CPack)

if(WIN32)
  set(PACKAGE_DIR ${CMAKE_BINARY_DIR}/${PACKAGE_FILE_NAME})

  # Add target to package and sign executable.
  add_custom_target(
    deploy
    signtool
    sign
    /v
    /f
    ${CERTIFICATE_FILE}
    /p
    ${CERTIFICATE_PASSWORD}
    ${CMAKE_BINARY_DIR}/${PACKAGE_FILE_NAME}.exe
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PACKAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/Gittyup.exe
            ${PACKAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/Gittyup.pdb
            ${PACKAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/indexer.exe
            ${PACKAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/indexer.pdb
            ${PACKAGE_DIR})

  add_dependencies(deploy package)
endif()
