set(CONF_DIR ${CMAKE_SOURCE_DIR}/conf)
set(RSRC_DIR ${CMAKE_SOURCE_DIR}/rsrc)

set(MAC $<PLATFORM_ID:Darwin>)

# Windows packaging is x64-only. (CMake's WIN32 is true for any Windows ABI,
# including obsolete 32-bit toolchains.)
set(GITTYUP_PACK_WIN64 OFF)
if(WIN32)
  if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Gittyup packaging: Windows is 64-bit (x64) only.")
  endif()
  set(GITTYUP_PACK_WIN64 ON)
endif()

# Install Qt plugins.
# QJpegPlugin is needed for image thumbnails; the SVG/ICO/GIF imageformat
# plugins and the SVG icon engine are required for the UI to render correctly.
set(QT_PLUGINS QJpegPlugin QSvgPlugin QICOPlugin QGifPlugin QSvgIconPlugin)

# Qt 6 moved the SSL/TLS implementation out of QtNetwork into a plugin. Without
# it every https request fails with "TLS initialization failed", which breaks
# the update check and the remote hosting accounts.
set(QT_PLUGINS ${QT_PLUGINS} QTlsBackendOpenSSLPlugin QTlsBackendCertOnlyPlugin)

if(APPLE)
  set(QT_PLUGINS ${QT_PLUGINS} QCocoaIntegrationPlugin QMacStylePlugin)
elseif(GITTYUP_PACK_WIN64)
  set(QT_PLUGINS ${QT_PLUGINS} QWindowsIntegrationPlugin
                 QModernWindowsStylePlugin QSchannelBackendPlugin)
else()
  set(QT_PLUGINS ${QT_PLUGINS} QXcbIntegrationPlugin
                 QComposePlatformInputContextPlugin)
endif()

if(GITTYUP_PACK_WIN64)
  set(INSTALL_LIBDIR ${CMAKE_INSTALL_BINDIR})
elseif(NOT APPLE)
  set(INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
else()
  set(INSTALL_LIBDIR ${CONTENTS_DIR})
endif()

if(FLATPAK)
  qt_import_plugins(gittyup INCLUDE ${QT_PLUGINS})
elseif(GITTYUP_PACK_WIN64 OR NOT USE_SYSTEM_QT)
  foreach(QT_PLUGIN ${QT_PLUGINS})
    if(NOT TARGET Qt6::${QT_PLUGIN})
      continue()
    endif()

    get_target_property(PLUGIN Qt6::${QT_PLUGIN} LOCATION_${CMAKE_BUILD_TYPE})
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

    if(GITTYUP_PACK_WIN64)
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
    get_target_property(QT_LIBRARY Qt6::${QT_MODULE}
                        LOCATION_${CMAKE_BUILD_TYPE})

    if(APPLE)
      get_filename_component(QT_FRAMEWORK ${QT_LIBRARY} DIRECTORY)

      # FIXME: QT_LIBRARY points directly to the library bin instead of the
      # symlink 2 directories above (since qt6)
      string(REGEX REPLACE "/Versions/.*" "" QT_FRAMEWORK "${QT_FRAMEWORK}")

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
      if(GITTYUP_PACK_WIN64)
        get_filename_component(TARGET_NAME ${QT_LIBRARY} NAME)

        # Copy into build dir.
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                ${QT_LIBRARY} ${CMAKE_BINARY_DIR})
      else()
        # Strip minor and patch version extensions.
        get_filename_component(TARGET_NAME_WE ${QT_LIBRARY} NAME_WE)
        set(TARGET_NAME ${TARGET_NAME_WE}.so.6)
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

      if(NOT GITTYUP_PACK_WIN64)
        # Delete runpath.
        install(CODE "execute_process(COMMAND chrpath --delete
            \${CMAKE_INSTALL_PREFIX}/${TARGET_NAME}
          )" COMPONENT ${GITTYUP_NAME})
      endif()
    endif()
  endforeach()
endif()

# Install SSL libraries.
if(NOT APPLE)
  # On Windows the installer must be self-contained. Qt's OpenSSL TLS backend
  # loads "libssl-3-x64.dll" / "libcrypto-3-x64.dll" at runtime; the system
  # OpenSSL install is frequently a *different* major version (e.g. 4.x) which Qt
  # will not load, so we bundle the 3.x DLLs that ship with the openssl submodule.
  if(GITTYUP_PACK_WIN64)
    set(_openssl_src "${CMAKE_SOURCE_DIR}/dep/openssl/openssl")
    foreach(_ssl_name libssl-3-x64 libcrypto-3-x64)
      set(_ssl_dll "${_openssl_src}/${_ssl_name}.dll")
      if(EXISTS "${_ssl_dll}")
        # Copy into build dir.
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_ssl_dll}"
                                ${CMAKE_BINARY_DIR})
        install(
          FILES "${_ssl_dll}"
          DESTINATION ${INSTALL_LIBDIR}
          PERMISSIONS
            OWNER_READ
            OWNER_WRITE
            OWNER_EXECUTE
            GROUP_READ
            GROUP_EXECUTE
            WORLD_READ
            WORLD_EXECUTE
          COMPONENT ${GITTYUP_NAME})
      endif()
    endforeach()

    # Qt support libraries required by the GUI (D3D/ANGLE software fallback).
    get_filename_component(_qt_bin_dir "${Qt6_DIR}/../../../bin" ABSOLUTE)
    foreach(_qt_support d3dcompiler_47 opengl32sw dxcompiler dxil)
      set(_support_dll "${_qt_bin_dir}/${_qt_support}.dll")
      if(EXISTS "${_support_dll}")
        install(
          FILES "${_support_dll}"
          DESTINATION ${INSTALL_LIBDIR}
          PERMISSIONS
            OWNER_READ
            OWNER_WRITE
            OWNER_EXECUTE
            GROUP_READ
            GROUP_EXECUTE
            WORLD_READ
            WORLD_EXECUTE
          COMPONENT ${GITTYUP_NAME})
      endif()
    endforeach()
  elseif(NOT USE_SYSTEM_OPENSSL)
    if(GITTYUP_PACK_WIN64)
      set(SSL_LIB_SUFFIX "-3-x64.dll")
    else()
      set(SSL_LIB_SUFFIX ".so.3")
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
        COMPONENT ${GITTYUP_NAME})
    endforeach()
  endif()

  # libssh2 (submodule) installs its DLL without COMPONENT ${GITTYUP_NAME};
  # CPack only installs that component, so ship the DLL from Gittyup's pack
  # rules. The imported target exists for both the system and the bundled build.
  if(GITTYUP_PACK_WIN64 AND TARGET libssh2::libssh2)
    get_target_property(_libssh2_dll libssh2::libssh2 LOCATION)
    if(_libssh2_dll AND EXISTS "${_libssh2_dll}")
      install(
        FILES "${_libssh2_dll}"
        DESTINATION ${INSTALL_LIBDIR}
        PERMISSIONS
          OWNER_READ
          OWNER_WRITE
          OWNER_EXECUTE
          GROUP_READ
          GROUP_EXECUTE
          WORLD_READ
          WORLD_EXECUTE
        COMPONENT ${GITTYUP_NAME})
    endif()
  endif()

  if(GITTYUP_PACK_WIN64 OR APPLE)
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
elseif(GITTYUP_PACK_WIN64)
  set(CPACK_GENERATOR NSIS)
  set(CPACK_NSIS_CREATE_ICONS_EXTRA
      "CreateShortCut \
    \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${GITTYUP_NAME}.lnk\\\" \
    \\\"$INSTDIR\\\\${CMAKE_INSTALL_BINDIR}\\\\${GITTYUP_EXECUTABLE_NAME}.exe\\\"
    CreateShortCut \
    \\\"$DESKTOP\\\\${GITTYUP_NAME}.lnk\\\" \
    \\\"$INSTDIR\\\\${CMAKE_INSTALL_BINDIR}\\\\${GITTYUP_EXECUTABLE_NAME}.exe\\\""
  )
  # Remove the desktop shortcut on uninstall so it does not linger.
  set(CPACK_NSIS_DELETE_ICONS_EXTRA
      "Delete \\\"$DESKTOP\\\\${GITTYUP_NAME}.lnk\\\"")
  set(CPACK_NSIS_EXECUTABLES_DIRECTORY ${CMAKE_INSTALL_BINDIR})
  set(CPACK_NSIS_MUI_FINISHPAGE_RUN ${GITTYUP_EXECUTABLE_NAME}.exe)

  install(
    FILES ${RSRC_DIR}/vcredist_x64.exe
    DESTINATION .
    COMPONENT ${GITTYUP_NAME})
  set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
      "ExecWait '\\\"$INSTDIR\\\\vcredist_x64.exe\\\" /q /norestart'")
  set(PLATFORM "-win64")
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
if(GITTYUP_PACK_WIN64)
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
endif()

set(CPACK_PACKAGE_FILE_NAME ${PACKAGE_FILE_NAME})
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_BINARY_DIR}/pack)
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${GITTYUP_NAME})
set(CPACK_PACKAGE_VENDOR "Gittyup Contributors")
set(CPACK_PACKAGE_VERSION_MAJOR ${GITTYUP_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${GITTYUP_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${GITTYUP_VERSION_PATCH})

if(GITTYUP_PACK_WIN64)
  set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE.md)
else()
  install(
    FILES ${CMAKE_SOURCE_DIR}/LICENSE.md
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/Gittyup
    RENAME LICENSE)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
include(CPack)

if(GITTYUP_PACK_WIN64)
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
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/gittyup-indexer.exe
            ${PACKAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/gittyup-indexer.pdb
            ${PACKAGE_DIR})

  add_dependencies(deploy package)
endif()
