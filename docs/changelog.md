### vX.X.X - 2023-04-20 (DEV)

Description

#### Added

* UI(Commit List): Added a right-click menu entry to rename branches.
* UI(Main Menu): Added a menu-entry to rename the current branch.
* UI(Diff View): Added line wrapping option in Tools - Options - Diff

#### Changed

* UI(Commit List): Collapse multiple branch and tag right-click menu entries
                   into submenus. This affects the checkout and delete operations.
* Fix(Build System): Force usage of clang-format v13 to ensure consistent formatting.

----

### v1.3.0 - 2023-04-20

Performance Improvement and feature release

#### Added

* Colorized status badges
* Template: use first template as default template for the commit message
* Search function for the treeview
* Reworked credential store: add possibility to choose between different methods to store credentials

#### Changed

* Fix external diff in Flatpak build
* Fix windows credentials
* Fix force push to correct remote
* Fix tab title if more than three times a repository with the same name is opened
* Fix storing repository settings correctly, because otherwise they are not applied
* Fix language support
* Improved refresh velocity
* Fix storing and restoring current opened file when Gittyup refreshes
* Improved velocity for files with many hunks

----

### v1.2.2 - 2023-01-22

Bug fix release

#### Changed

* Fix flatpak install process

----

### v1.2.1 - 2023-01-22

Bug fix release

#### Added
* Possibility to hide avatar (Settings - Window - View - Show Avatars)
* Show log entry when a conflict during rebase happens

#### Changed

* Fix download url for flatpak and macos
* Fix Segmentation fault when ignoring files
* Fix discard of complete files and submodules
* Fix context menu entries
* Fix bytesize overflow
* Fix focus loose during scrolling in the Commitlist with the keyboard
* Do not crash when the repository is for some reason broken
* Fix crash if rebasing is not possible

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
