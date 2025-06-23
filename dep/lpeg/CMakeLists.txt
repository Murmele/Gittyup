set(LPEG_DIR lpeg-1.1.0)

add_library(lpeg ${LPEG_DIR}/lpcap.c ${LPEG_DIR}/lpcode.c ${LPEG_DIR}/lpprint.c
                 ${LPEG_DIR}/lpcset.c ${LPEG_DIR}/lptree.c ${LPEG_DIR}/lpvm.c)

target_link_libraries(lpeg lua)

target_include_directories(lpeg
                           INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/${LPEG_DIR})
set_property(TARGET lpeg PROPERTY C_STANDARD 99)
