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

### v2.6.3 - 2020-07-14

#### Added

* Added external tool definitions for Beyond Compare 4. (Stefan Knotzer)
* Added option to disable translation. (Stefan Knotzer)
* Added options to delete branches and tags from the commit list context menu. (Stefan Knotzer)
* Added button to the clone dialog to navigate to a local directory. (Stefan Knotzer)
* Added Brazilian Portuguese translation. (Francisco)

#### Fixed

* Fixed incorrect setting change when checking the "update submodules after pull" check box.
* Fixed failure to stage some renames. (Maickonn Richard)
* Fixed crash on right-click when a both the status diff and another commit are selected. (Stefan Knotzer)
* Fixed the definition of an identifier in indexed search to include a dash followed a digit.
* (Mac) Fixed failure to load translations.

----

### v2.6.2 - 2020-05-03

#### Added

* Added Spanish translation. (Gustavo Velasco-Hernandez)
* Added command line flag (--no-translation) to disable automatic loading of translation files.
* Added option to create a squash merge (git merge --squash). (Maickonn Richard)
* Added settings to specify SHH config and key file paths. (Kas)
* Added settings for prompt to stage directories and large files. (Maickonn Richard)

#### Fixed

* Fixed crash on Fetch All without an open repo. (Maickonn Richard)
* (Mac) Fixed crash on window close.
* (Linux) Fixed failure to quit from the menu. (Kas)

#### Changed

* Changed layout of diff view to more clearly group hunks with their file.
* (Mac) Quit when the last window is closed.

----

### v2.6.1 - 2019-12-26

#### Fixed

* Fixed potential crash on adding a remote account from the sidebar.
* Fixed width of ID column in compact history mode for some fonts.
* (Mac) Fixed missing translations for some items in app menu.
* (Mac) Fixed incorrect UI language when English comes before German in list of preferred languages.

----

### v2.6.0 - 2019-12-12

#### Added

* Added options to automatically prune remote tracking branches on fetch/pull. (Kas)
* Added context menu action to push tags to the default remote. (Kas)
* Added prompt to delete tags (including from the remote). (Kas)
* Added German language translation. (Kas)
* Added theme variables to color the tab bar independently. (Kas)
* Added compact history mode (in the gear popup menu). (Attila Greguss)

#### Fixed

* Jump to matching reference on click instead of selection change.
* Fixed possible invalid commit editor message after revert, cherry-pick, etc.
* Fixed inconsistent history list font size on some displays.
* Fixed failure to cancel some remote transfer operations.
* Removed artificial limitation on merging and rebasing with a detached HEAD.
* Fixed crash when merging or rebasing with a detached HEAD. (Kas)

----

### v2.5.11 - 2019-10-30

#### Added

* Added context menu item to remove remote account from the sidebar.
* Added usage reporting setting to the 'General' preferences panel.
* Remember sidebar remote account expansion state.
* Unique sidebar and open tab names in the same way as recent repositories.
* Added 'Fetch All' item to 'Remote' menu.

#### Fixed

* Fixed failure to launch external edit tools with spaces in their path.
* Disallow pasting rich text into the commit editor.
* (Win) Fixed several issues relating to symlinks.

#### Changed

* Allow more than 99 columns in line length plugin interface.
* Clarify copyright statement in the about dialog.
* (Mac) Enable the hardened runtime to allow notarization on macOS Catalina.

----

### v2.5.10 - 2019-08-29

#### Added

* Automatically switch between light and dark mode on macOS Mojave when using the native theme.

#### Fixed

* Fixed stash performance issue.
* Fixed crash on LFS-not-found error.
* Avoid reporting the LFS-not-found error multiple times.
* Fixed possible failure to look up the correct SSH identity file when the config file contains a 'HostName' entry.
* Fixed failure to draw tree view badges in some cases.
* Add missing dark mode colors for some lexers that reference colors directly (e.g. markdown).

----

### v2.5.9 - 2019-08-10

#### Fixed

* Updated Bitbucket integration to use v2.0 API.
* Avoid opening a new tab for repositories that are already open.
* Changed new branch dialog default start point to the HEAD instead of the first branch in the reference list.
* Fix regression in staging submodules.
* Fix crash on right-click on clean status diff.
* Fix regression in double-clicking to clone a repository.

#### Changed

* Changed wording of prompt-to-amend dialog to clarify its purpose.

----

### v2.5.8 - 2019-07-17

#### Added

* Added menu items and keyboard shortcuts to stage and unstage all files.

#### Fixed

* Fixed possible crash in merge dialog on action change.
* Fixed performance regression on viewing status diffs with large files.
* (Linux) Fixed input of accented characters when using the compose input method.
* (Linux) Fixed several credential helper issues.

#### Changed

* Double-clicking on a repo in the sidebar no longer opens a new tab if the repo is already open.
* Apply "store credentials in secure storage" setting immediately.
* Allow name and date to wrap when the detail view becomes too small to show them side-by-side.

----

### v2.5.7 - 2019-05-22

#### Fixed

* Fixed possible index corruption.

----

### v2.5.6 - 2019-05-16

#### Added

* Added option to ignore whitespace in diffs.
* Remember previously opened repository path and use it when opening new repositories.

#### Fixed

* Fixed failure push a branch to a different remote when it's already up-to-date with its upstream.

#### Changed

* Revert to the system list view selection color in the native theme.

----

### v2.5.5 - 2019-03-19

#### Added

* Added syntax highlighting for SQL and Erlang files.

#### Fixed

* Reload submodules on refresh and when the HEAD changes.
* Fixed failure to refresh GitHub accounts with more than one page of repositories.
* (Linux) Fixed spurious refresh on modification of some ignored files.

----

### v2.5.4 - 2019-02-25

#### Added

* Allow checking out remote branches from the commit list.
* Added option to reset remote branch to an existing local branch on checkout.

#### Fixed

* Quote path arguemnts to filters.
* Fixed crash on 'Find' when no repository is open.
* Fixed file context menu performance issue by enabling navigation items unconditionally.
* Fixed failure to get more than 20 repositories from GitLab.
* Work around lack of support for the 'no\_proxy' environment variable in libgit2.

#### Changed

* Map 'Refresh' to the platform-specific shortcut.

----

### v2.5.3 - 2019-01-24

#### Added

* Added support for ed25519 SSH identity files.

#### Fixed

* Fixed possible crash after starred commits have been added and removed.

----

### v2.5.2 - 2019-01-06

#### Fixed

* (Win) Fixed status diff performance regression on indexes built with older versions of GitAhead.
* (Win) Fixed installer shortcuts to be relative to the actual install location instead of hard coded to the default location.

----

### v2.5.1 - 2018-12-22

#### Fixed

* (Win) Restored license agreement to the installer.
* (Win) Avoid restart after the VS redistributable is installed.
* (Mac) Fixed failure to launch on macOS 10.12 and 10.13.

#### Changed

* Truncate tool bar badge numbers at 999.

----

### v2.5.0 - 2018-12-13

#### Changed

* GitAhead is now open source and free for everybody! See the new gitahead.com for details.

----

### v2.4.7 - 2018-11-20

#### Added

* (Mac) Added basic support for touch bar.
* (Win) Added support for symlinks when core.symlinks=true.

#### Fixed

* Fixed inverted plus/minus color in default theme.
* Disallow multiple remote accounts of the same kind with the same username.

----

### v2.4.6 - 2018-09-26

#### Fixed

* Fixed crash on initializing new repository.
* Fixed port conflict with some web servers.

----

### v2.4.5 - 2018-09-24

#### Fixed

* Fixed performance issues on some large repositories.
* (Mac) Fixed compatibility issues with macOS Mojave.

#### Changed

* Changed default merge operation to commit.

----

### v2.4.4 - 2018-09-11

#### Fixed

* Improved performance on some very large repositories.
* (Win) Fixed regression in looking up credentials in some cases.

### v2.4.3 - 2018-08-31

#### Fixed

* Fixed possible crash on fetch.
* Fixed additional issue in extracting repo name from URL.
* Fixed performance issue when browsing tree view.
* Fixed describe performance issues on very large repositories.
* (Linux) Fixed failure to lookup username from credential helper.

#### Changed

* Changed default font sizes slightly.
* Changed external editor setting to set "gui.editor" instead of "core.editor".
* Changed credential helper implementation to be more consistent with git.
* (Linux) Execute application in context of user's default shell when launched from desktop.

----

### v2.4.2 - 2018-08-23

#### Added

* Added context menu item to rename branches.
* Added option to set external editor (core.editor).
* Respect GIT_EDITOR, core.editor, VISUAL, and EDITOR (in that order) when editing in an external editor.

#### Fixed

* Fixed failure to rename branches.
* Fixed clone dialog repo name detection to use everything up to .git.

#### Changed

* Moved external tools options into their own settings page.

----

### v2.4.1 - 2018-08-17

#### Added

* Added option to disable credential storage.

#### Fixed

* Fixed remote host connection issues.

----

### v2.4.0 - 2018-07-20

#### Added

* Added empty main window interface for opening repositories.
* Added options to merge dialog to require or disable fast-forward.

#### Fixed

* Fixed possible file corruption when discarding or staging individual hunks.
* Fixed intermittent invalid status diff and possible crash.

#### Changed

* Show repository sidebar by default.
* Allow last tab to be closed.
* Start application with an empty main window instead of repository chooser.
* Remember open tabs when exiting the app by closing the last window.

----

### v2.3.7 - 2018-07-10

#### Fixed

* Fixed possible hang on start.
* Fixed regression in adding recent repositories with the same name as an existing repository.

----

### v2.3.6 - 2018-07-05

#### Added

* Added option to stop prompting to stage directories and large files.
* (Beta) Added repository sidebar.

#### Fixed

* Improved performance of staging a large number of files.
* Added missing [remote rejected] push errors to the log.
* (Win) Copy file paths with native separators.

#### Changed

* Increased threshold for prompt to stage large files by a factor of 10.

----

### v2.3.5 - 2018-06-21

#### Fixed

* Fixed ignore issues on some patterns.
* Fixed performance issue when opening settings dialogs.
* Fixed possible crash on changing window settings after a window has been closed.
* Improve commit list performance when sorting by date and the graph is disabled.

#### Changed

* Made the diff view drop shadow more subtle.

----

### v2.3.4 - 2018-06-14

#### Fixed

* Fixed selection issues and possible crash on reference view filtering.

#### Changed

* Don't show the license error dialog on application start when the error is the result of a network error.
* Avoid connecting to remote accounts every time the repository chooser is opened.

----

### v2.3.3 - 2018-05-27

#### Added

* Added plugin API to get hunk lexer.

#### Fixed

* Fixed interactivity during rebase.
* Fixed line length plugin issue on some empty lines.
* (Mac) Fixed style regression on macOS native theme.

#### Changed

* Disabled most plugin checks on unstyled hunks.
* Automatically add /api/v3 to GitHub Enterprise URLs.
* Removed bundle git-lfs. LFS integration requires git-lfs to be installed separately.


----

### v2.3.2 - 2018-05-14

#### Added

* Added command line option to populate the pathspec filter.
* Added notes about authentication with GitHub and GitLab by personal access token.

#### Fixed

* Fixed inverted tab plugin options.
* Disable pull on bare repositories.
* Fixed spurious prompts to decrypt SSH identity file after invalid public key connection error.
* (Win) Fixed treatment of junction points to match command line git.

----

### v2.3.1 - 2018-05-03

#### Added

* Added context menu item to set or unset the executable mode of files after they have been staged. This is equivalent to the 'git update-index --chmod=(+|-)x' command.

#### Fixed

* Fixed file discard button to remove directories.
* Fixed trailing whitespace plugin error for files that have multiple line ending characters (e.g. CRLF).
* Fixed file context menu performance issue.
* (Linux) Fixed failure to detect libsecret at runtime on Ubuntu 18.04.

----

### v2.3.0 - 2018-05-01

#### Added

* Added plugin API to flag errors in diffs.
  * Added several built-in plugins.
  * API documentation can be found at Help -> Plugin Documentation...
* Prompt before staging large binary files.
* Added option to create and clone repositories bare.
* Added option to deinitialize LFS for the repository.

#### Fixed

* Fixed failure to prompt for HTTPS credentials when the remote URL scheme is 'http'.

----

### v2.2.2 - 2018-04-18

#### Added

* Added menu item to remove all LFS locks.

#### Fixed

* Fixed failure to execute LFS filters when git-lfs isn't on the PATH.
* (Win) Fixed indexer crash regression.

#### Changed

* Store all commit list options as repository-specific settings instead of global settings.
* Changed the behavior of the reference interface to keep the HEAD as the current reference when 'show all branches' is enabled.

----

### v2.2.1 - 2018-04-10

#### Added

* Added size label for binary files in the diff view.
* Added before and after version of images in the diff view.
* Added commit list context menu items to check out local branches that point to the selected commit.

#### Fixed

* Improved performance of filtering by pathspec when the search index is enabled.
* Fixed failure to execute LFS pre-push hook when git-lfs isn't on the PATH.
* Fixed performance regression when opening the config dialog.
* (Win) Fixed badge color of unselected items in the dark theme.

----

### v2.2.0 - 2018-03-27

#### Added

* Added improved support for LFS.
* Show icon of non-image binary files in diff view.
* (Win) Enable smooth scaling by automatically detecting HiDPI displays.

#### Fixed

* Fixed failure to fully stage symlinks.
* Fixed failure to refresh correctly after amending the first commit.

----

### v2.1.3 - 2018-03-06

#### Fixed

* Fixed failure to clone when the URL contains leading or trailing whitespace.
* Fixed failure to identify the correct SSH key file when using an ssh: style URL.
* Fixed unexpected quit after adding a valid license when the evaluation is expired.
* (Win) Fixed intermittent indexer crash.

----

### v2.1.2 - 2018-03-01

#### Added

* Added privacy statement.

#### Fixed

* Fixed crash on conflicted diffs after all conflicts have been resolved.
* Fixed possible indexer crash.
* (Win) Fixed failure to connect to GitHub through HTTPS on Windows 7.

----

### v2.1.1 - 2018-02-27

#### Added

* Allow 'Remove Untracked Files' context menu actions to remove untracked directories.
* (Win) Generate crash dump file when the indexer process crashes.

#### Changed

* Show commit list times in the local time zone. The detail view still shows time in the author's time zone offset from UTC.

----

### v2.1.0 - 2018-02-15

#### Added

* Allow tags to be pushed individually.
* Added context menu option to clean untracked files from the working directory.
* Automatically scale down images that are wider than the diff area.
* Added options to collapse added and deleted files. All files now start expanded by default.
* (Linux) Added libsecret credential storage for distros that have it.

#### Fixed

* Fixed regression in visiting range links from the log.
* Print error to log when the pre-push hook fails.
* Print error to log when stage fails because of a filter error.
* Fixed staged state of files that can't be staged because of a filter error.
* Fixed failure to execute some filters (including the LFS ones) that require the working directory be set to the working directory of the repository.
* Fixed interface issues in some themes.
* Fixed failure to update graph during refresh when the refresh is triggered by changes to the working directory.

#### Changed

* Changed selection in the commit list to allow only two commits to be selected with Ctrl+click (or Cmd+click on Mac) instead of a contiguous range with Shift+click. Selecting a contigous range doesn't make sense when it includes commits on different branches.

----

### v2.0.6 - 2018-01-23

#### Fixed

* Disable editing and hide caret on untracked files in the diff view.
* Fixed failure to cancel remote transfer in some cases.
* Fixed failure to report error after invalid offline registration attempt.

----

### v2.0.5 - 2018-01-17

#### Added

* Added theme menu item to edit the current theme.

#### Fixed

* Fixed crash on some diffs.
* (Win) Fixed issues with localized repository and config path names.

#### Changed

* Changed HEAD reference color in dark theme.

----

### v2.0.4 - 2018-01-10

#### Added

* Allow more SSL errors to be ignored.
* Remember ignored SSL errors and don't continue to prompt about them.
* (Mac) Added 'Open in GitAhead' services context menu action.

#### Fixed

* Fixed crash on conflicted files that only exist on one side of the merge.
* (Win) Install updates in the previous install location.

----

### v2.0.3 - 2018-01-05

#### Added

* Remember 'no commit' setting in the merge dialog.
* Added more specific error message on failure to establish SSL connection.
* Allow certain SSL errors to be ignored.
* (Win) Added explorer context menu shortcuts to installer.

#### Fixed

* Fixed integration with GitLab. A personal access token is now required for authentication.

----

### v2.0.2 - 2017-12-22

#### Fixed

* Fixed issues with offline licenses.

----

### v2.0.1 - 2017-12-20

#### Added

* Added menu indicator to 'Pull' button. The right third of the button now pops up the menu.
* Added option to disable filtering of non-existant paths from the recent repository list.
* Respect the 'gui.encoding' setting when loading and saving diff and editor text.
* Added option to set the character encoding in the 'Diff' configuration panels.
* (Win) Allow most external tools to run without shell evaluation.

#### Fixed

* Fixed deselection by Ctrl+click in the file list.
* (Win) Fixed HEAD reference selection color in commit list.

#### Changed

* Changed several theme keys to be more consistent. Added another sample theme.

----

### v2.0.0 - 2017-12-14

#### <span style='color: red'>GitAhead 2.0 requires a new license!</span>

* All current commercial users can upgrade their license at no cost. Check your email for details.
* Non-commercial users can sign up for a free non-commercial account through a link in the app.
* The two week evaluation period will reset for all users, so a new license isn't required immediately.
* If you have any questions please contact us at [support@gitahead.com](mailto:support@gitahead.com).

#### Added

* Added support for custom themes.

#### Fixed

* Fixed failure to cleanup after conflict resolution in some cases.

----

### v1.4.14 - 2017-12-11

#### Fixed

* Fixed regression in selecting the status (uncommitted changes) row after the working directory changes.
* Fixed failure to persist conflict resolution selections after refresh.

----

### v1.4.13 - 2017-12-06

#### Fixed

* Fixed possible crash on newly initialized repos.
* Fixed crash on submodule update after fetch when new submodules are added.

#### Changed

* Disallow automatic update to new major versions.

----

### v1.4.12 - 2017-11-30

#### Added

* Added tag icon next to tag reference badges.
* Added option to show the full repository path in the title bar.

#### Fixed

* Remember the size and location of the start dialog.
* Reset selection more consistently to the HEAD branch when it changes.

----

### v1.4.11 - 2017-11-21

#### Added

* Added buttons to conflict hunks to choose 'ours' or 'theirs' changes.
* Disallow staging conflicted files that still contain conflict markers.
* Log all merge abort actions.
* Added log text to explain conflict resolution process.

#### Fixed

* Fixed failure to show stashes or possible crash in some cases.
* Fixed regression in updating commit list reference badges when references are added and deleted.

### v1.4.10 - 2017-11-10

#### Added

* Changed background color of conflict badges.
* Added status text to indicate the number of unresolved conflicts.
* Added conflict hunk background colors on ours and theirs lines.

#### Fixed

* Fixed issue with spaces in .gitignore files.
* Fixed a small memory leak.
* Fixed conflict hunk check state.
* Fixed conflict hunk line number issue.
* (Mac) Fixed issue that prevented the disk image from mounting on systems older than macOS 10.12.

----

### v1.4.9 - 2017-11-06

#### Added

* Added context menu action to remove multiple untracked files at once.
* Show detached HEAD label at a specific tag if any tags point directly to the commit.

#### Fixed

* Fixed cancel during resolve.
* Fixed regression is showing the detached HEAD label in the commit list.
* Fixed failure to immediately index some commits if the indexer is running while references are updated.
* Fixed failure to get more than the first page of GitHub repositories.

----

### v1.4.8 - 2017-10-30

#### Added

* Added prompt before staging directories.
* Substitute emoji for emoji short codes in commit messages.
* Added missing extension mapping for CoffeeScript.

#### Fixed

* Sort conflicts to the top unconditionally.
* Automatically select the status row when the HEAD changes.
* Fixed commit list scroll performance on repositories with many refs.
* (Win) Search for git bash in default install location.
* (Win) Added warning dialog when an external tool fails because bash can't be found.

#### Changed

* Changed the behavior of double-click in the file list to open the external diff/merge tool if one is enabled.

----

### v1.4.7 - 2017-10-20

#### Added

* Added context menu actions to apply, pop, and drop stashes.

#### Fixed

* Fixed issues related to revert and cherry-pick that result in a conflict.
* Fixed merge abort menu item to update more consistently.

----

### v1.4.6 - 2017-10-17

#### Added

* Added advanced option to specify a custom URL for hosting providers.

#### Fixed

* Fixed failure to execute pre-push hook (including LFS) in some cases.
* Fixed error messages on console.

----

### v1.4.5 - 2017-10-10

#### Added

* Added option to merge without committing.
* Added context menu actions to stage/unstage multiple files at once.

#### Fixed

* Fixed usability issues in the custom external tools editor interface.
* Don't yield focus to the commit message editor when staging files with the keyboard.
* Fixed state of stash toolbar button and menu item when the working directory is clean except for untracked files.
* Fixed failure to cherry-pick and revert when the working directory is dirty.
* Fixed hang on some diffs.
* Fixed garbage remote transfer rate when the elapsed time is too small to measure.
* (Mac) Fixed several issues on macOS 10.13.

----

### v1.4.4 - 2017-09-22

#### Added

* Style editor content after saving a new file.

#### Fixed

* Fixed style of tag reference badges that point to the HEAD.
* Fixed regression in showing images in the diff area.
* Fixed regression in indexing by lexical class.
* (Win) Fixed regression in staging "exectuable" files.

----

### v1.4.3 - 2017-09-15

#### Added

* Added reference badge for the detached HEAD state.
* Added commit list context menu item to rebase a specific commit.
* Added remote branch context menu item to create a new local branch that tracks the remote branch.

#### Fixed

* Fixed bugs in staging file mode changes.
* Fixed failure to look up the correct host name for SSH URLs that use an alias from the SSH config file.

#### Changed

* Start indexer process with lower priority.

----

### v1.4.2 - 2017-09-11

#### Added

* Draw HEAD reference badges darker and bold.
* Added dotted graph edge from the status item to the current HEAD commit.
* Added an option to show the status item even when the workin directory is clean.
* Added commit list context menu item to merge a specific commit.

----

### v1.4.1 - 2017-09-05

#### Added

* Added merge and rebase context menu actions to reference view references.

#### Fixed

* Fixed SSH host name lookup to use the first matching host instead of the last.
* Fixed regression in loading the initial diff on repo open when the working directory scan is very fast.
* (Win) Fixed failure to look up stored passwords from the credential manager for HTTPS URLs that don't include the username.

----

### v1.4.0 - 2017-08-30

#### Added

* Added context menu action to delete tags.
* Added option to show all references in the commit list.
* Added option to change commit list sort order.
* Added option to turn off the commit list graph.
* Expand the reference view by clicking in its label area.
* Jump to the selected reference by clicking on its name in the reference view.

#### Fixed

* Fixed dark theme appearance of the advanced search button.
* Don't reset the care position/selection when the editor window loses focus.

#### Changed

* Changing the selected reference back to the current (HEAD) branch no longer causes the working directory to be rescanned.

----

### v1.3.13 - 2017-08-18

#### Fixed

* Fixed file list scroll range bug.
* Fixed unresponsive interface on pull followed by submodule update.
* Fixed invalid commit list context menu actions when multiple commits are selected.
* Fixed diff view update bug when selecting a different branch in the reference view.
* Disable diff view expand buttons for files that don't have any hunks.
* Fixed file context menu crash in a newly initialized repository.
* Fixed indexer crash on failure to diff binary blobs that don't appear to be binary based on the heuristic.
* Fixed crash when the .git/gitahead directory gets created with the wrong permissions.

#### Added

* Added options to sort file list ascending or descending.
* Added commit list context menu option to star commits.
* Added commit list context menu action to create a new branch at an arbitrary commit.
* Checkout on double-click in the reference view.

#### Changed

* Show renamed and copied files expanded by default in diff view.

----

### v1.3.12 - 2017-08-04

#### Fixed

* Fixed intermittent indexer crash.
* (Win) Fixed failure to install indexer component.
* Fixed external diff/merge tool display bug on paths containing spaces.

#### Added

* Added option to sort file list and diff view by status instead of name.

----

### v1.3.11 - 2017-07-20

#### Fixed

* Fixed missing status badges in tree view.
* Fixed search index locking issue.
* Clean up search index temporary files after crash.
* Fixed performance issue on some repositories.

#### Added

* Added syntax highlighting for .less files.
* Added basic support for LFS clean/smudge filter and pre-push hook.
* (Linux) Install .desktop file and icons for better desktop integration.

----

### v1.3.10 - 2017-07-06

#### Added

* Added ability to search by wildcard query.
* Added ability search date ranges with before: and after: operators.

#### Changed

* The search index now stores dates in a locale-agnostic format.
* The indexer now runs as an external worker process.

----

### v1.3.9 - 2017-06-26

#### Fixed

* Elide reference badges in the detail view instead of forcing the window to grow.
* Update reference badges in the detail view when a reference is added/deleted/updated.
* Fixed crash in checkout dialog when there aren't any references.
* Fixed failure to push from 'Push To...' dialog when opened from a log link.
* Push to the remote reference that corresponds to the current branch's upstream by default.

#### Added

* Added clear button to all fields in the advanced search popup.

----

### v1.3.8 - 2017-06-21

#### Fixed

* Fixed failure to connect to the update server since v1.3.6.

#### Added

* Added file context menu action to open files in the default file browser.

----

### v1.3.7 - 2017-06-19

#### Fixed

* Disallow removing command line tools that weren't installed by the app.
* Delete remote branch by the name of the upstream instead of the name of the local branch.

#### Added

* Added an interface for creating annotated tags.
* Show annotated tag author and message in hover text in the reference view.
* Added context menu item to delete local branches from the reference view.
* Added explanatory tool tips to the advanced search popup.
* Added an advanced option in the 'Push To...' dialog to push to a different remote name.

----

### v1.3.6 - 2017-06-12

#### Fixed

* Fixed failure to remember the new default repository path when accepting the init repo dialog.
* Fixed regression in refreshing the status diff after creating a new file in the working directory.

#### Added

* Added init/deinit column to submodule configuration table.
* Allow context menu to discard changes to deleted files.
* Enable merging and rebasing on remote branches and tags.
* Added new push hints for the case where no remotes are configured for the current branch.

----

### v1.3.5 - 2017-05-30

#### Fixed

* Fixed staged state of dirty submodules. Disable dirty submodule check boxes.
* Fixed crash on submodule update when there are modules that have been inited but don't exist on disk.
* (Win) Fixed search completion popup layout.
* (Win) Fixed intermittent crash when files in ignored directories change on disk.

#### Added

* Added context menu item to discard changes in selected files.
* (Linux) Added command line installer.

#### Changed

* Allow remote host accounts to be collapsed in the repository browser.
* Moved new branch dialog upstream option into an advanced settings section.

----

### v1.3.4 - 2017-05-23

#### Fixed

* Fixed automatic scaling issue.

----

### v1.3.3 - 2017-05-22

#### Added

* Added automatic refresh when the working directory changes. Removed refresh on activation.
* Added prepopulated commit message when staging files and the commit message editor is empty.

#### Fixed

* Fixed the initial directory of the clone dialog browse button.
* Fixed crash when staging/unstaging the hunk of an untracked file.
* (Win) Scale application uniformly (although less smoothly) when font scaling is greater than 100%.

#### Changed

* Show the "Commit" button as the default button.
* Double-click in the commit list no longer opens a new tab.

----

### v1.3.2 - 2017-05-10

#### Added

* Added dialog to prompt the user to choose a theme on the first run.

#### Fixed

* Fixed performance issues on some large repositories.
* Fixed commit list graph and reference bugs on some repositories.
* Fixed several dialog default buttons in the dark theme.
* Fixed possible inconsistent state of the search index if the write is interrupted.

----

### v1.3.1 - 2017-05-02

#### Added

* Added a reference badge in the detail view that shows the number of commits since the latest tag. This is similar to git describe.
* Added gnome-keyring support for securely storing passwords on Linux.
* Added copy button next to the short id in the detail view.
* Added automatic refresh when the application loses and regains focus.
* Added inline autocomplete to pathspec filter field.

#### Fixed

* Fixed crash on refresh after a tab is closed.
* Fixed indexer performance issues.
* Fixed possible failure to respect indexer term limits.

#### Changed

* Reimplemented clone/init dialog as a wizard to walk through each step.

----

### v1.3.0 - 2017-04-21

#### Added

* Added redesigned interface for choosing the current reference and setting the pathspec filter.

#### Fixed

* Fixed crash on submodule init.
* Fixed click issue in the star area of the "uncommitted changes" row.

----

### v1.2.6 - 2017-04-13

#### Added

* Added ability to star favorite commits.
* Added option to delete branch from remote when deleting a local branch.

#### Fixed

* Don't collapse log entries when clicking on a link in the log.
* Fixed possible crash during indexing.
* (Linux) Fixed possible crash on start.

----

### v1.2.5 - 2017-04-05

#### Added

* Added a dark theme.

#### Fixed

* Fixed possible crash during indexing.
* (Win) Fixed failure to start with a valid evaluation license in some cases.

----

### v1.2.4 - 2017-03-15

#### Added

* Show kind of reference in the reference list label.

#### Fixed

* Fixed possible crash on diffs with no newline at the end of the file.

----

### v1.2.3 - 2017-03-10

#### Added

* Highlight changed words in diff output.
* Added advanced search button to the bottom of the search completion popup.
* Added prompt to add a new local branch when checking out a remote tracking branch.
* Respect 'GIT\_SSL\_NO\_VERIFY' environment variable and 'http.sslVerify' config setting when validating certificates.

#### Fixed

* Fixed diff error near the end of files with no trailing newline.
* Fixed possible destructive changes when clicking on a stale 'abort merge' link.
* Fixed incorrect error message when a merge fails because of conflicts with uncommitted changes.

----

### v1.2.2 - 2017-03-02

#### Added

* Added drag-and-drop into the diff area to copy files into the repository.
* Added menu action to create new files with the built-in editor.
* Added interface for creating new files and copying files into newly initialized repositories.
* Added initial repository setup actions to repository chooser. Made the remote host actions more obviously clickable.

#### Fixed

* Fixed regression in staging directories.
* Restore missing file context menu items.
* Fixed failure to amend the very first commit in a repository.
* Fixed failure to load the correct image file from the selected commit.
* (Mac) Fixed regression in scrolling with the scroll wheel over untracked files.

----

### v1.2.1 - 2017-02-24

#### Added

* Added basic GitLab integration.

#### Fixed

* Fixed performance issues in commit editor.
* Fixed crash when adding a new branch from the config dialog in a newly initialized repository.
* Fixed crash when clicking on the 'Edit Config File...' button in the global settings dialog and the global settings file (~/.gitconfig) doesn't exist.
* Open remote account dialog with the correct kind when double-clicking on one of the account kinds in the repository chooser.

----

### v1.2.0 - 2017-02-17

#### Added

* Added external edit, diff, and merge context menu actions.

#### Fixed

* Disable minus button when an error node is selected in the repository chooser remote list.
* Fixed wrong error message in license error dialog.
* (Win) Fixed advanced search button position.

#### Changed

* Changed settings and config dialogs to reflect newly added external tool settings.

----

### v1.1.4 - 2017-02-04

#### Fixed

* Improved performance when selecting uncommitted changes with modified submodules.

#### Changed

* (Linux) Changed application style to be more consistent.

----

### v1.1.3 - 2017-01-31

#### Fixed

* Fixed failure to stage/unstage deleted files.

----

### v1.1.2 - 2017-01-25

#### Fixed

* Fixed pathological log view performance on checkout when many files are modified.
* Fixed layout of reference badges in commit list.
* (Win) Fixed clipped file name in diff view.

----

### v1.1.1 - 2017-01-16

#### Fixed

* (Linux) Fixed 'invalid SSL certificate' error.
* (Win) Fixed broken context menu on right-click on selected files in the file list.

----

### v1.1.0 - 2017-01-05

#### Added

* Added new reference popup with a tabbed interface and a field to filter by name.

#### Fixed

* Fixed failure to remove config section headers when the last key is removed.
* Queue up new fetch, pull, push and submodule update requests when an existing asynchronous operation is ongoing instead of silently dropping them.

----

### v1.0.3 - 2016-12-27

#### Fixed

* Fixed failure to authenticate with SSH when the public key file is missing.
* Fixed failure to remove recent repositories when clicking the minus button.
* Fixed crash on navigating forward through history to the HEAD branch when there are no uncommitted changes.

----

### v1.0.2 - 2016-12-06

#### Added

* Added missing syntax highlight file extension mappings for several languages.
* Added more specific error message for cases where the SSH identity file isn't found.

#### Fixed

* (Win) Fixed slight visual issue with multiple items selected in the commit list.

----

### v1.0.1 - 2016-12-02

#### Added

* Remember the maximum size of the file list when the splitter is moved.
* Added missing Fortran file extension mappings.

#### Fixed

* Fixed several history issues.
* Fixed failure to authenticate with SSH key when the server also supports username/password authentication.

----

### v1.0.0 - 2016-11-28

#### Fixed

* Fixed several conflict workflow issues.
* Fixed failure to prompt to save on editor close.
* Fixed crash on diff after files change on disk.
* Fixed several file context menu issues.
* Fixed some cases of pathological performance on filter by pathspec.
* (Mac) Fixed failure to select the current item in the pathspec popup when pressing return/enter.

#### Added

* Split conflicted files into hunks around conflict markers.
* Abort merge with reset --merge semantics instead of --hard semantics.
* Truncate blame message text with ellipsis instead of clipping at the bottom.

----

### v0.9.3 - 2016-11-14

#### Fixed

* Fixed wrong error message in some cases.
* Fixed regression when staging or discarding hunks.
* (Win) Fixed proxy issue when connecting to remotes.

----

### v0.9.2 - 2016-11-09

#### Added

* Added diff view context menu option to filter history by the selected path.

#### Fixed

* Fixed issues related to reading the merge message from disk.
* Fixed regression in scrolling to find matches in the diff view.

#### Changed

* Changed pathspec placeholder text.

----

### v0.9.1 - 2016-11-06

#### Added

* Order all conflicts first.
* Show the merge head in the commit list when merging.
* Added checkout context menu action to the diff view.

#### Fixed

* Only store credentials in the keychain after successful remote connection.
* Fixed commit list order for commits that happen at the same time (e.g. because of rebase).
* Fixed file list scroll bugs.
* Fixed crash on editor open in newly initialized repos.
* Fixed merge workflow to allow conflicts to be resolved and merges committed.

#### Changed

* Changed the meaning of selection in the file list to filter the diff view by the selected files.

----

### v0.9.0 - 2016-10-31

#### Added

* Added licensing scheme. Contact us to beta test a permanent license.
* Allow tabs to be reordered by dragging.
* Respect branch.&lt;name&gt;.rebase and pull.rebase when pulling.
* Search up through parent directories when opening a directory that doesn't contain a repository.

#### Fixed

* Disallow invalid branch names in the new branch dialog.
* Disable discard buttons for submodules.
* Read ~/.ssh/config to determine which identity file to load.
* Fixed password save regression.
* Fixed incorrect line endings after discarding hunks with filter settings (e.g. core.autocrlf).

#### Changed

* Disable warning about opening an invalid repository from the command line. It's more annoying than useful.

----

### v0.8.15 - 2016-10-18

#### Added

* Added ability to select and copy from the log view as both plain and rich text.
* Added drag-and-drop of repository folders into the main window to open in a new tab.
* (Mac) Added drag-and-drop of repository folders into the application icon to open.
* (Win) Added menu bar to editor windows. Introduces several missing shortcuts.

#### Fixed

* Fixed editor automatic scroll regressions.
* (Win) Fixed editor find regression.

----

### v0.8.14 - 2016-10-13

#### Added

* Added initial Linux beta build.

#### Fixed

* Fixed additional proxy auto-detection issue.
* Fixed some HTTPS authentication issues and incorrect errors.

----

### v0.8.13 - 2016-09-30

#### Added

* Auto-detect system proxy settings.
* (Win) Added VS2013 redistributables to the installer.
* (Win) Removed component page from the installer.
* (Win) Added check box to launch the application after the installer finishes.

#### Fixed

* Start merge/revert/cherry-pick/stash dialog without keyboard focus in the message editor.
* Fixed hang or bad performance on some hunks at the very edge of showing the horizontal scroll bar.
* Fixed failure to resize hunks after font size changes.
* Fixed failure to update submodules after pull when the pull results in a merge.
* Fixed loss of search filter results after a branch is updated.
* Fixed crash on newly initialized repositories with untracked files.
* Select head reference after the current reference is deleted.

----

### v0.8.12 - 2016-09-23

#### Added

* Added fake credentials implementation for Windows.

#### Fixed

* Fixed regression in commit list update after a remote reference changes.
* Fixed config dialog performance issue on repositories with many submodules.
* Fixed size of untracked file hunks in diff view.
* Fixed stale status diff after aborting a merge.
* (Win) Fixed stash crash on when no default signature is set.
* (Win) Fixed persistent file lock and several issues related to it.

----

### v0.8.11 - 2016-09-13

#### Added

* Added new Repository menu with Commit and Amend Commit actions.

#### Fixed

* Improved layout of long file names in diff view.
* Changed behavior of hunk check box to only collapse the hunk when clicked.
* Fixed performance issue during commit message editing with lots of staged files.

----

### v0.8.10 - 2016-08-30

#### Fixed

* Fixed possible crash on remote transfer cancel.
* Fixed regression in setting branch upstream from config dialog.
* Improved responsiveness of changing tabs and some other operations.
* Fixed hunks sometimes starting out collapsed after commit.
* Fixed commit list update after the current branch's upstream changes.
* Fixed responsiveness issues when the commit list changes.
* Fixed possible remote transfer thread hang after prompting for HTTPS credentials.
* (Win) Fixed crash on reference list navigation with arrow keys.

----

### v0.8.9 - 2016-08-23

#### Added

* Added submodule update dialog to update a subset of submodules and set parameters.
* Added Ok button to dismiss settings dialog.

#### Fixed

* Fixed intermittent crash during status calculation.
* Restore correct reference from history and log links.
* Improved responsiveness and memory usage on files with a large number of hunks.

----

### v0.8.8 - 2016-08-12

#### Added

* Update submodules recursively be default.

#### Fixed

* Fixed authentication hang when ssh-agent is running but doesn't have any keys.
* Fixed horizontal scroll of log view.
* Fixed layout and input validation in new branch dialog.
* Fixed crash on repositories that contain remote branches that don't refer to a valid remote.

----

### v0.8.7 - 2016-08-05

#### Added

* Enable reset for the head commit (e.g. for hard reset).
* Improved responsiveness after staging/unstaging with many modified/untracked files.
* Restore commit list selection after update instead of always selecting the first row.

#### Fixed

* Fixed failure to fetch annotated tags added to existing commits.
* Fixed commit list sometimes switching to newly added references instead of updating the currently selected reference.
* Fixed possible deadlock caused by a race between remote transfer cancel and credential lookup.
* Fixed regression in setting initial focus on the commit list.
* Fixed possible black background in blame margin.
* Fixed possible crash on tab close.
* Fixed checkout check box visibility regression in new branch dialog.

----

### v0.8.6 - 2016-07-29

#### Added

* Added tabbed interface.

#### Fixed

* Fixed binary detection and loading of untracked image files in the diff view.
* Fixed visual artifacts and warning message in the blame margin when the repository contains a single commit.

----

### v0.8.5 - 2016-07-18

#### Added

* Changed file context copy menu to include short, relative and long names.
* Added option to show a heat map in the blame margin (enabled by default).

#### Fixed

* Fixed crash on opening a bare repository.
* Fixed bare repository name in the title bar.
* Fixed crash on starting an asynchronous remote operation when another is already running.
* Fixed failure to reset scroll width after loading a new file in the editor.

----

### v0.8.4 - 2016-07-13

#### Added

* Load advanced search query items from the query string.
* Added completion suggestions for each field of the advanced search widget.
* Added context menu items to copy file name and path.
* Added option to update submodules after pull (disabled by default).
* (Win) Added basic crash logging.

----

### v0.8.3 - 2016-07-08

#### Fixed

* Disable branch checkout and creation when the HEAD is unborn.
* Fixed regression in staging untracked files.
* Fixed status and staging of files in newly inited repos.
* Allow commit on an unborn HEAD.

#### Added

* Show the unborn HEAD name in the title bar.
* Show the commit id in the title bar when the HEAD is detached.
* Added a 'Checkout' command to the commit list context menu.
* Added option to automatically fetch periodically (enabled by default).
* Added option to automatically push after commit (disabled by default).

----

### v0.8.2 - 2016-07-01

#### Fixed

* Fixed submodule update hang after the first submodule.
* Fixed crash when the Submodule->Open menu is activated without and active window.
* Fixed possible corruption of the git index when a checkout is triggerd during search indexing.
* (Win) Fixed initial location of settings dialog when opened at a specific index.

#### Added

* Allow hunks to be discarded individually.
* Allow about dialog version string to be selected.
* Added option to push all tags.

----

### v0.8.1 - 2016-06-28

#### Fixed

* Fixed crash on status diff cancel.

----

### v0.8.0 - 2016-06-28

#### Added

* Hunks can now be staged/unstaged individually.
* Added delete confirmation dialogs before deleting remotes and branches.
* Added warning when deleting branches that aren't fully merged.

#### Fixed

* Disallow deleting the current branch.
* Disallow accepting new remote dialog with empty name or URL.

#### Known Issues

* Blame doesn't update after changes to the editor buffer.
* Pathspec field auto-complete only works for the first path element.
* (Win) Editor windows don't have a menu bar or key bindings.

----

### v0.7.9 - 2016-06-10

#### Added

* Group remote branches by remote in the reference popup.
* Added hunk header edit buttons start editing at the location of the hunk.
* Added cherry-pick context menu action.
* Added repository special state (e.g. MERGING, REBASING, etc.) to the title bar.

#### Fixed

* Fixed regression in reporting errors from remote connections.
* (Win) Fixed crash when opening new editor windows.
* (Win) Fixed tree expansion indicator style changing after showing dialogs.

----

### v0.7.8 - 2016-06-05

#### Added

* Added parameter to 'Fetch/Pull From' dialogs to update existing tags.
* Connect to remotes asynchronously and throttle log updates to fix performance issues.
* Added option to init and update submodules.
* Added username field to HTTPS credentials prompt.
* Changed commit editor status text to include the total number of files.
* Added progress indicator and cancel button to asynchronous log entries.
* Allow clone to be canceled by clicking on the cancel button.
* Allow status diff to be canceled by clicking on the cancel button.

#### Fixed

* Fixed spurious update notification for existing tags that aren't really updated by fetch.
* Fixed line stats (plus/minus) widget resize issue.
* Fixed submodule update to ignore modules that haven't been inited.
* Removed superfluous timestamps from clone dialog log area.
* Fixed initial size of config dialog when opening it up at a specific index.

----

### v0.7.7 - 2016-05-29

#### Added

* Generate fake user name and email when they're not in settings.
* Added log hints to set user name and email and amend commit.
* Added repository configuration interface for general settings.
* Added global git configuration interface to global preferences.
* Added commit editor status label to indicate number of staged files.
* Added draggable splitter between file list and diff view.
* Added swift syntax highlighting.

#### Fixed

* Fixed initial visibility of staged files.

----

### v0.7.6 - 2016-05-24

#### Added

* Added better error reporting for failure to commit

#### Fixed

* Fixed check state of newly added files in the diff view
* Hide tree view during status update
* Fixed crash on submodule open when the submodule hasn't been inited

----

### v0.7.5 - 2016-05-23

#### Added

* Added warning prompt before trying to commit on a detached HEAD
* Added commit message sumary to log entries for commit and rebase
* Added markdown syntax highlighting
* Log stash and stash pop operations
* Added double-click to open submodule in tree view, file list and diff view
* Added fetch/pull/push from/to actions with a dialog to choose parameters
* Added log hints to set upstream branch on push and allow push without upstream

#### Fixed

* Collapse existing log entries when new top-level entries are added instead of when the log is made visible
* Keep log open while editing merge commit message

----

### v0.7.4 - 2016-05-16

#### Added

* Added submodule config panel
* Added submodule menu and changed layout and shortcuts of some existing menus
* Added submodule update
* Added link to open submodules from diff view
* Added tag context menu action in the commit list

#### Fixed

* Fixed spurious conflict on rebase with modified submodules
* Fixed dirty submodules always shown as staged
* Fixed toolbar remote button badge update when the HEAD is detached or the current branch doesn't have an upstream
* Removed all synchronization between the indexer and the main thread by abandoning the notion of restarting a running indexer. Now the main thread signals the indexer to cancel and waits for it to finish before restarting.

----

### v0.7.3 - 2016-05-05

#### Added

* Added disambiguation of identically named repositories in recent repository lists
* Added option (with shortcut) to checkout the currently selected branch
* Added daily check for updates when automatic update is enabled
* Added support for showing multiple roots in the commit list
* Show the upstream branch associated with a local branch even when it's not reachable from the local ref

#### Fixed

* Fixed lookup of credentials by percent encoded username (e.g. email address)
* Fixed deadlock on pull caused by a race between indexing starting after fetch and canceling indexing before fast-forward

----

### v0.7.2 - 2016-05-02

#### Added

* Added refresh item to view menu

#### Fixed

* Fixed performance issues related to large numbers of untracked/modified files
* (Mac) Removed install rpaths from Qt frameworks to placate Gatekeeper
* (Mac) Changed update mount point to a guaranteed unique temporary directory. Fixes update failure when the original download image is still mounted.

----

### v0.7.1 - 2016-04-28

#### Fixed

* Updated libgit2 to fix a large memory leak

----

### v0.7.0 - 2016-04-26

#### Known Issues

* Hunks can't be staged individually even though they have a check box
* Dirty submodules always show up as staged
* Pathspec field auto-complete only works for the first path element
* Blame doesn't update changed editor buffers
* (Windows) The style of tree expansion indicators sometimes changes after showing dialogs
* There are several other bugs and missing features in the issue tracker
* This list is by no means exhaustive...
