if(APPLE)
  set(INSTALLER_IMPL_FILE Installer_mac.mm)
  set(INSTALLER_IMPL_LIBS "-framework Security")
elseif(UNIX)
  set(INSTALLER_IMPL_FILE Installer_linux.cpp)
endif()

add_library(cli Installer.cpp ${INSTALLER_IMPL_FILE})

target_link_libraries(cli Qt5::Core ${INSTALLER_IMPL_LIBS})
