Scintilla is fetched automatically by CMake (see `FetchContent_Declare(scintilla ...)`
in CMakeLists.txt). To update to a new version, check
https://www.scintilla.org/ScintillaHistory.html for the latest release number.

Download the archive from SourceForge (`https://downloads.sourceforge.net/project/scintilla/scintilla/<version>/scintilla<version-no-dots>.tgz`),
not from `www.scintilla.org` directly. The latter is fronted by a CDN that
reliably returns HTTP 403 to non-browser HTTP clients such as
flatpak-builder's downloader, which obviously breakes Flatpak build in the CI.
NOTE: Both `FetchContent_Declare(scintilla ...)` and the `scintilla` archive source in
`../../com.github.Murmele.Gittyup.yml` must use the same SourceForge URL and be
updated together, with the sha256 in the manifest updated to match the new
archive.
