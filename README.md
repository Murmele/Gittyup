[![Gittyup Status](https://github.com/Murmele/Gittyup/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/Murmele/Gittyup/actions/workflows/build.yml)
[![Matrix](https://img.shields.io/matrix/Gittyup:matrix.org?label=Matrix%20Chat)](https://matrix.to/#/#Gittyup:matrix.org)
[![Donate Liberapay](https://liberapay.com/assets/widgets/donate.svg)](https://liberapay.com/Gittyup/donate)

<a href="https://flathub.org/apps/details/com.github.Murmele.Gittyup">
<img
    src="https://flathub.org/assets/badges/flathub-badge-i-en.png"
    alt="Download Gittyup on Flathub"
    width="240px"/>
</a>

Gittyup
==================================

Gittyup is a graphical Git client designed to help you understand and manage your source code history. The [latest stable release](https://github.com/Murmele/Gittyup/releases/latest)
is available either as pre-built flatpak for Linux, 32 / 64 binary for Windows, macOS,
or can be built from source by following the directions [below](https://github.com/Murmele/Gittyup#how-to-build).

The [latest development version](https://github.com/Murmele/Gittyup/releases/tag/development) is available pre-built as well.

Gittyup is a continuation of the [GitAhead](https://github.com/gitahead/gitahead) client.

![Gittyup](https://raw.githubusercontent.com/Murmele/Gittyup/master/rsrc/screenshots/main_dark_orig.png)

Table of contents
=================
<!--ts-->
   * [Features](#features)
   * [How to Get Help](#how-to-get-help)
   * [Build Environment](#build-environment)
   * [Dependencies](#dependencies)
   * [How to Build](#how-to-build)
   * [How to Install](#how-to-install)
      * [Flatpak from terminal](#flatpak-from-terminal)
   * [How to Contribute](#how-to-contribute)
   * [License](#license)
<!--te-->

Features
---------------
To get an overview of the current features please have a look at the [GitHub Page](https://murmele.github.io/Gittyup/)

How to Get Help
---------------

Ask questions about building or using Gittyup on
[Stack Overflow](http://stackoverflow.com/questions/tagged/gittyup) by
including the `gittyup` tag. Remember to search for existing questions
before creating a new one.

Report bugs in Gittyup by opening an issue in the
[issue tracker](https://github.com/Murmele/gittyup/issues).
Remember to search for existing issues before creating a new one.

If you still need help, check out our Matrix channel
[Gittyup:matrix.org](https://matrix.to/#/#Gittyup:matrix.org).

Build Environment
-----------------

* C++11 compiler
  * Windows - MSVC >= 2017 recommended
  * Linux - GCC >= 6.2 recommended
  * macOS - Xcode >= 10.1 recommended
* CMake >= 3.3.1
* Ninja (optional)

Dependencies
------------

External dependencies can be satisfied by system libraries or installed
separately. Included dependencies are submodules of this repository. Some
submodules are optional or may also be satisfied by system libraries.

**External Dependencies**

* Qt (required >= 5.12)

**Included Dependencies**

* libgit2 (required)
* cmark (required)
* git (only needed for the credential helpers)
* libssh2 (needed by `libgit2` for SSH support)
* openssl (needed by `libssh2` and `libgit2` on some platforms)

Note that building `OpenSSL` on Windows requires `Perl` and `NASM`.

How to Build
------------

**Initialize Submodules**

    git submodule init
    git submodule update

**Build OpenSSL**

    # Start from root of gittyup repo.
    cd dep/openssl/openssl

Windows:

    perl Configure VC-WIN64A
    nmake

macOS (Intel):

    ./Configure darwin64-x86_64-cc no-shared
    make
    
macOS (Apple Silicon)

    ./Configure darwin64-arm64-cc no-shared
    make
    
Linux:

    ./config -fPIC
    make

**Configure Build**

    # Start from root of gittyup repo.
    mkdir -p build/release
    cd build/release
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ../..

If you have Qt installed in a non-standard location, you may have to
specify the path to Qt by passing `-DCMAKE_PREFIX_PATH=<path-to-qt>`
where `<path-to-qt>` points to the Qt install directory that contains
`bin`, `lib`, etc.

**Build**
```
    ninja
```
    
### A Convenient Shell Script for Ubuntu is available [here](https://raw.githubusercontent.com/Murmele/Gittyup/master/pack/buildUbuntu.sh), and will install all the necessary prerequisites, and build a release version for immediate use.

How to Install
-----------------
### Linux

The easiest way to install Gittyup is by using [Flatpak](https://flathub.org/apps/details/com.github.Murmele.Gittyup).

**Arch Linux**

Install the `gittyup` package from the Arch User Repository.

	git clone https://aur.archlinux.org/gittyup.git
	cd gittyup
	makepkg -si

Or use an AUR helper.
Install `gittyup-git` for the VCS build.

### Mac OS

**Homebrew**

Install the `gittyup` cask from [Homebrew](https://formulae.brew.sh/cask/gittyup).

    brew install gittyup

### Flatpak from terminal

If you want a more pure console use, this script run flatpak version disowning the process and silence the output pushing it to /dev/null.
Just save the script somewhere in your path, for example `/usr/bin` (or `~/.local/bin` if you have exported it), give execution permissions `chmod +x`, and run `gittyup` from your terminal.

```bash
#!/bin/bash
DIR=$(dirname "${BASH_SOURCE[0]}")
function run_disown() {
    "$@" & disown
}
function run_disown_silence(){
    run_disown "$@" 1>/dev/null 2>/dev/null
}
run_disown_silence flatpak run com.github.Murmele.Gittyup
```

How to Contribute
-----------------

We welcome contributions of all kinds, including bug fixes, new features,
documentation and translations. By contributing, you agree to release
your contributions under the terms of the license.

Contribute by following the typical
[GitHub workflow](https://docs.github.com/en/get-started/quickstart/github-flow)
for pull requests. Fork the repository and make changes on a new named
branch. Create pull requests against the `master` branch. Follow the
[seven guidelines](https://chris.beams.io/posts/git-commit/) to writing a
great commit message.

License
-------

Gittyup and its predecessor GitAhead are licensed under the MIT license. See LICENSE.md for details.
