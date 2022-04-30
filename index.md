<!--- Github page main file --->

Gittyup is a graphical Git client designed to help you understand and manage your source code history. Gittyup is an open source software developed by voluntiers, if you like the application please support us [![Donate Liberapay](https://liberapay.com/assets/widgets/donate.svg)](https://liberapay.com/Gittyup/donate)

The release version is available for
- [flatpak for Linux](https://github.com/Murmele/Gittyup/releases/download/stable/Gittyup.flatpak)
- [32](https://github.com/Murmele/Gittyup/releases/download/stable/Gittyup-win32-1.1.0.exe) / [64](https://github.com/Murmele/Gittyup/releases/download/stable/Gittyup-win64-1.1.0.exe) binary for Windows


The [development version](https://github.com/Murmele/Gittyup/releases) is available either as pre-built for
- [flatpak for Linux](https://github.com/Murmele/Gittyup/releases/download/latest/Gittyup.flatpak),
- [32](https://github.com/Murmele/Gittyup/releases/download/latest/Gittyup-win32-1.1.0-dev.exe) / [64](https://github.com/Murmele/Gittyup/releases/download/latest/Gittyup-win64-1.1.0-dev.exe) binary for Windows

or, can be built from source by following the directions in the [Gittyup Repository](https://github.com/Murmele/Gittyup#how-to-build).

To see the changes of the current version please have a look at the <A href="#changelog">changelog</A> section

![Gittyup](https://raw.githubusercontent.com/Murmele/Gittyup/master/rsrc/screenshots/main_dark_orig.png)

How to Get Help
===============

Ask questions about building or using Gittyup on
[Stack Overflow](http://stackoverflow.com/questions/tagged/gittyup) by
including the `gittyup` tag. Remember to search for existing questions
before creating a new one.

Report bugs in Gittyup by opening an issue in the
[issue tracker](https://github.com/Murmele/gittyup/issues).
Remember to search for existing issues before creating a new one.

If you still need help, check out our Matrix channel
[Gittyup:martix.org](https://matrix.to/#/#Gittyup:matrix.org).

Features
========

### Single branch view to focus on your work
Select "Show Selected Branch" in the drop down menu above the commit list
![Single branch](https://raw.githubusercontent.com/Murmele/Gittyup/master/rsrc/screenshots/main_show_selected_branch.png)

### Single line staging 
by eighter clicking on the checkboxes next to each line or by selecting the relevant code and pressing "S". For unstaging you can uncheck the checkboxes or press "U". To revert changes, select the text and press "R".

![Single line staging](https://raw.githubusercontent.com/Murmele/Gittyup/master/rsrc/screenshots/double_treeview_single_line_staging.png)

### Fullscreen
of the history or the change dialog by pressing Ctrl+M

### Staring commits
to find specific commits much faster
![Staring commits](https://raw.githubusercontent.com/Murmele/Gittyup/master/rsrc/screenshots/starring_commits.png)

### Tag selection
Use an existing tag as template for your next tag. So you never have to look which is your latest tag

![Tag selection](https://raw.githubusercontent.com/Murmele/Gittyup/master/rsrc/screenshots/tag_selection.png)

### Tree View
To visit the blame with its history for unchanged files

![Tree View](https://raw.githubusercontent.com/Murmele/Gittyup/master/rsrc/screenshots/treeview.png)

### Blame View
See blame of the current version with an integrated timeline to see who changed which line

### Commit message template
Create you commit messages according a defined template

![Commit message template selection](https://raw.githubusercontent.com/Murmele/Gittyup/master/rsrc/screenshots/CommitMessageTemplateSelection.png)

![Commit message template editor](https://raw.githubusercontent.com/Murmele/Gittyup/master/rsrc/screenshots/CommitMessageTemplateEditor.png)

### Tabs
to be able to switch fast between repositories

### And a lot more ...

Changelog
=========

{% include_relative changelog.md %}

