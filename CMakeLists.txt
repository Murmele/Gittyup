cmake_minimum_required(VERSION 3.12)
project(Gittyup)

# Set name and version.
set(GITTYUP_NAME "Gittyup")
set(GITTYUP_EXECUTABLE_NAME "gittyup")
set(GITTYUP_IDENTIFIER "com.github.Murmele.Gittyup")
set(GITTYUP_VERSION_MAJOR 1)
set(GITTYUP_VERSION_MINOR 4)
set(GITTYUP_VERSION_PATCH 0)
set(GITTYUP_VERSION
    "${GITTYUP_VERSION_MAJOR}.${GITTYUP_VERSION_MINOR}.${GITTYUP_VERSION_PATCH}"
)

string(TIMESTAMP CURR_YEAR "%Y")
add_compile_definitions(CURR_YEAR=${CURR_YEAR})
configure_file(${CMAKE_SOURCE_DIR}/LICENSE.md.in ${CMAKE_SOURCE_DIR}/LICENSE.md
               @ONLY NEWLINE_STYLE UNIX)

# Write version to file so it can be used also from external, for example in the
# github manifest
file(WRITE "${CMAKE_BINARY_DIR}/Version.txt" ${GITTYUP_VERSION})

set(DEV_BUILD
    ""
    CACHE STRING
          "Mark this build as a development build with the given description")

if(DEV_BUILD)
  set(BUILD_DESCRIPTION " (development build: ${DEV_BUILD})")
else()
  set(BUILD_DESCRIPTION "")
endif()

# Generate build date.
string(TIMESTAMP GITTYUP_BUILD_DATE)

# Lookup git revision.
find_package(Git)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} --git-dir=${CMAKE_SOURCE_DIR}/.git show -s
            --format=%h HEAD
    OUTPUT_VARIABLE GITTYUP_BUILD_REVISION
    OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
  set(GITTYUP_BUILD_REVISION "unknown")
endif()

# Explicitly disable shared libraries.
set(BUILD_SHARED_LIBS OFF)

option(FLATPAK "Building for flatpak" OFF)
option(DEBUG_FLATPAK "Building but using flatpak urls for testing" OFF)
option(ENABLE_UPDATE_OVER_GUI "Enable updating from the Gittyup gui" ON)
option(USE_SYSTEM_OPENSSL "Use the system-wide OpenSSL installation" OFF)
option(
  USE_SYSTEM_LIBGIT2
  "Use the system-wide libgit2 installation (Gittyup requires the current development branch)"
  OFF)
option(USE_SYSTEM_LIBSSH2 "Use the system-wide libssh2 installation" OFF)
option(USE_SYSTEM_GIT "Use the system-wide GIT installation" OFF)
option(USE_SYSTEM_QT "Don't copy QT files into the installation directory" OFF)
option(USE_SYSTEM_LUA "Use the system-wide Lua installation" OFF)
option(USE_SYSTEM_HUNSPELL "Use the system-wide hunspell installation" OFF)
option(USE_SYSTEM_CMARK "Use the system-wide cmark installation" OFF)
option(ENABLE_TESTS "Enable Gittyup Unittests" ON)

set(LUA_MODULES_PATH
    CACHE
      PATH
      "Path to the directory with native Lua modules (only relevant if system-wide Lua installation is used)"
)

if (NOT USE_SYSTEM_LIBSSH2)
  set(USE_SSH "localbuild")
endif()

# Require C++17.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if(HAIKU)
  # qsort_r in libgit2 requires this
  set(CMAKE_EXE_LINKER_FLAGS -lgnu)
elseif(UNIX AND NOT APPLE)
  set(CMAKE_EXE_LINKER_FLAGS -ldl)
endif()

# Find Qt modules.
set(QT_MODULES
    Concurrent
    Core
    Gui
    Network
    PrintSupport
    Widgets
    Test)

if(UNIX)
  set(QT_MODULES ${QT_MODULES} DBus)
endif()

option(DEBUG_OUTPUT
       "Print debug output (Only available if debug menu is enabled!)" ON)
option(DEBUG_OUTPUT_GENERAL "Enable general debug messages" ON)
option(DEBUG_OUTPUT_REFRESH "Enable debug messages to debug the refresh flow"
       OFF)
if(NOT DEBUG_OUTPUT)
  add_compile_definitions(QT_NO_DEBUG_OUTPUT)
else()
  if(DEBUG_OUTPUT_GENERAL)
    add_compile_definitions(DEBUG_OUTPUT_GENERAL)
  endif()
  if(DEBUG_OUTPUT_REFRESH)
    add_compile_definitions(DEBUG_OUTPUT_REFRESH)
  endif()
endif()

find_package(
  Qt5 5.12
  COMPONENTS ${QT_MODULES} LinguistTools
  REQUIRED)
if(FLATPAK)
  find_package(
    Qt5 5.15
    COMPONENTS XcbQpa
    REQUIRED)
  add_compile_definitions(FLATPAK)
endif()

if(ENABLE_UPDATE_OVER_GUI)
  add_compile_definitions(ENABLE_UPDATE_OVER_GUI)
endif()

if(DEBUG_FLATPAK)
  add_compile_definitions(DEBUG_FLATPAK)
endif()

include(GNUInstallDirs) # Defines some variables as BINDIR, LIBDIR, ...

set(QT_TRANSLATIONS_DIR "${Qt5_DIR}/../../../translations")
set(QT_TRANSLATIONS_DIR "/usr/share/qt/translations")

if(APPLE)
  foreach(QT_MODULE ${QT_MODULES})
    # FIXME: Force debug build to link against release libraries inside of
    # frameworks on macOS. The Qt debug libraries are broken because their link
    # dependencies are to the release libraries (e.g. QtGui_debug depends on
    # QtCore). This causes multiple symbol definition errors at application load
    # time.
    get_target_property(LOCATION Qt5::${QT_MODULE} LOCATION)
    set_target_properties(Qt5::${QT_MODULE} PROPERTIES IMPORTED_LOCATION_DEBUG
                                                       ${LOCATION})
  endforeach()
endif()

if(APPLE)
  set(CONTENTS_DIR ${GITTYUP_EXECUTABLE_NAME}.app/Contents)
  set(RESOURCES_DIR ${CONTENTS_DIR}/Resources)
  set(L10N_INSTALL_DIR ${RESOURCES_DIR}/l10n)
elseif(UNIX)
  set(L10N_INSTALL_DIR ${CMAKE_INSTALL_LOCALEDIR}/${GITTYUP_NAME})
  set(RESOURCES_DIR ${CMAKE_INSTALL_DATADIR}/${GITTYUP_NAME})
else()
  set(L10N_INSTALL_DIR Resources/l10n)
  set(RESOURCES_DIR Resources)
endif()

add_subdirectory(dep)
add_subdirectory(src)
add_subdirectory(l10n)
add_subdirectory(pack)

if(ENABLE_TESTS)
  add_subdirectory(test)
endif()
