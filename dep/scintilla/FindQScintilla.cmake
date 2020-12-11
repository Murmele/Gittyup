
#This is free and unencumbered software released into the public domain.

#Anyone is free to copy, modify, publish, use, compile, sell, or distribute this software, either in source code form or as a compiled binary, for any purpose, commercial or non-commercial, and by any means.

#In jurisdictions that recognize copyright laws, the author or authors of this software dedicate any and all copyright interest in the software to the public domain. We make this dedication for the benefit of the public at large and to the detriment of our heirs and successors. We intend this dedication to be an overt act of relinquishment in perpetuity of all present and future rights to this software under copyright law.

#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#For more information, please refer to <https://unlicense.org/>

pkg_check_modules(QScintilla qscintilla2)

list(GET Qt5Core_INCLUDE_DIRS 0 Qt5_INCLUDE_DIRS)

if(QScintilla_FOUND)
	set("QScintilla_INCLUDE_DIRS" "${QScintilla_INCLUDE_DIRS}")
	set("QScintilla_LINK_LIBRARIES" "${QScintilla_LINK_LIBRARIES}")
else()
	find_library(
		QScintilla_LINK_LIBRARIES
		NAMES "qscintilla2_qt5"
		HINTS "/usr/lib/" "${CMAKE_INSTALL_FULL_LIBDIR}"
	)
	message(STATUS "QScintilla_LINK_LIBRARIES:${QScintilla_LINK_LIBRARIES}")
	find_path(
		QScintilla_INCLUDE_DIRS
		NAMES "qscilexerxml.h"
		HINTS "${Qt5_INCLUDE_DIRS}/Qsci/" "${CMAKE_INSTALL_FULL_INCLUDEDIR}"
		DOC "QScintilla2 headers"
	)
endif()

if("${QScintilla_INCLUDE_DIRS}" STREQUAL "QScintilla_INCLUDE_DIRS-NOTFOUND" OR "${QScintilla_LINK_LIBRARIES}" STREQUAL "QScintilla_LINK_LIBRARIES-NOTFOUND")
	set(QScintilla_FOUND OFF)
else()
	set(QScintilla_FOUND ON)
	message(STATUS "QScintilla found: ${QScintilla_INCLUDE_DIRS} ${QScintilla_LINK_LIBRARIES}")
	set(QScintilla_INCLUDE_DIRS "${QScintilla_INCLUDE_DIRS}" PARENT_SCOPE)
	set(QScintilla_LINK_LIBRARIES "${QScintilla_LINK_LIBRARIES}" PARENT_SCOPE)
	set("QScintilla_VERSION" "2")
endif()

set(QScintilla_FOUND "${QScintilla_FOUND}" PARENT_SCOPE)