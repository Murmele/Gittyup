### v1.2.1 - 2022-11-10

Bug fix release

### Added
* Possibility to hide avatar (Settings - Window - View - Show Avatars)

#### Changed

* Fix download url for flatpak and macos
* Fix Segmentation fault when ignoring files
* Fix discard of complete files and submodules
* Fix bytesize overflow

----

### v1.2.0 - 2022-10-28

Bug fix and feature release

#### Added
* Add support for solving merge conflicts for whole files
* Solving binary conflicts directly in Gittyup
* Support rebasing with conflict solving
* Implement amending commits
* Possibility to init submodules after clone (Settings - General - Update submodules after pull and clone)
* Hiding menu bar (Application Settings - Window - Hide Menubar)
* Implement support for Gitea instances

#### Changed
* Fix Segmentation fault when using space to stage files
* Fix menubar color in dark theme
* Filter only branches, tags, remotes attached to selected commit
* Fix crash when global GIT config is invalid
* Fix crash when having errors while adding a remote account
* Fix updater on windows, macos and linux (flatpak)
* Fix discarding file leading to discarding submodule changes
* Fix rebase log messages during rebase
* Improve SSH config handling
* Application settings and repository settings can now be selected with a single settings button
* Use the full file context menu for the staging file list
* Fix Arch Linux build

----

### v1.1.2 - 2022-08-12

Bug fix release

#### Changed

* Fix bundled OpenSSL version incompatibility

----

### v1.1.1 - 2022-06-09

Bug fix release

#### Added
* Distinguish between commit author and committer
* Show image preview also for deleted files
* Official macOS release
* Show which kind of merge conflict occurred for each conflict

#### Changed
* Fix single line staging if not all hunks are loaded
* Fix cherrypick commit author
* Fix segmentation fault if submodule update fails
* Fix line staging with windows new lines
* Show first change in the diff view when loading
* Improved windows icon

----

### v1.1.0 - 2022-04-30

Second release of Gittyup

#### Added
* Button to directly access the terminal and the filebrowser
* Add support for running in single instance mode
* Customizable hotkeys
* Quick commit author overriding
* keyboard-interactive SSH auth
* Improved single line staging and replacing staging image to a more appropriate one
* Font customizing
* Options to switch between staging/unstaging treeview, single tree view and list view
* Do not automatically abort rebase if conflicts occur
* Add possibility to save file of any version on local system
* Add possibility to open a file of any version with default editor

----

### v1.0.0 - 2021-11-18

First version of the GitAhead Fork Gittyup

#### Added
* Staging of single lines
* Double tree view: Seeing staged and unstaged changes in different trees.
* Maximize History or Diff view by pressing Ctrl+M
* Ignore Pattern: Ability to ignore all files defined by a pattern instead of only one file
* Tag Viewer: When creating a new tag all available tags are visible. Makes it easier to create consistent tags.
* Commit Message template: Making it easier to write template based commit messages.

----
