# For testing enable
#set(CMAKE_SOURCE_DIR <PATH TO SOURCE>)
#set(CMAKE_BINARY_DIR <PATH TO BINARY DIR>)
#set(DOC_SOURCE_DIR ${CMAKE_SOURCE_DIR}/docs)
#set(DOC_BINARY_DIR ${CMAKE_BINARY_DIR}/docs)
#set(CHANGELOG_HTML ${DOC_BINARY_DIR}/changelog.html)
#set(APPDATA_CONF ${CMAKE_SOURCE_DIR}/rsrc/linux/com.github.Murmele.Gittyup.appdata.xml.in)
#set(APPDATA ${CMAKE_BINARY_DIR}/rsrc/linux/com.github.Murmele.Gittyup.appdata.xml)

# add release notes to the appdata file
file(READ "${CHANGELOG_HTML}" HTML_CHANGELOGS)
# it is not allowed to have multiple texts without being in an environment
# So the description will be used for it
#string(REGEX REPLACE "<p>([^<]*)<\\/p>" "\\1" RELEASES ${HTML_CHANGELOGS}) # remove paragraph environment. In the metainfo all description titles must be paragraphs <p>
string(REGEX REPLACE "<h4>([A-Za-z0-9]*)<\\/h4>" "<p>\\1</p>" RELEASES ${HTML_CHANGELOGS}) # h4 is unknow to appdata so change it to a paragraph environment
string(REPLACE "\n" "\n\t" RELEASES ${RELEASES}) # add tabulator
# For Dev Version "vX.X.X - <current date> (DEV)" can be used to show in the changelog the current progress
string(REGEX REPLACE "<h3>(v[1-9]\\.[0-9]\\.[0-9]|vX\\.X\\.X) - ([0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9])( \\(DEV\\))?<\\/h3>" "<release version='\\1' date='\\2'>\n\t<description>" RELEASES ${RELEASES})
string(REGEX REPLACE "<hr \\/>" "</description>\n\t</release>" RELEASES ${RELEASES})
configure_file(${APPDATA_CONF} ${APPDATA})
