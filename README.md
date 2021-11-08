[![Actions Status](https://github.com/Murmele/gitahead/workflows/GitAhead%20%28master%29/badge.svg)](https://github.com/Murmele/gitahead/actions) [![Actions Status](https://github.com/Murmele/gitahead/workflows/GitAhead%20%28stage%29/badge.svg)](https://github.com/Murmele/gitahead/actions)


Gittyup
==================================

Gittyup is a graphical Git client designed to help you understand
and manage your source code history. It's available as a [pre-built
binary for Windows, Linux, and macOS](https://gitahead.github.io/gitahead.com/), or can be built from source by
following the directions below.

Gittyup is a continuation of the [GitAhead](https://github.com/gitahead/gitahead) client.

How to Get Help
---------------

Ask questions about building or using Gittyup on
[Stack Overflow](http://stackoverflow.com/questions/tagged/gitahead) by
including the `gitahead` tag. Remember to search for existing questions
before creating a new one.

Report bugs in Gittyup by opening an issue in the
[issue tracker](https://github.com/Murmele/gitahead/issues).
Remember to search for existing issues before creating a new one.

If you still need help, check out our Matrix channel
[GitAheadFork:martix.org](https://matrix.to/#/#Gittyup:matrix.org).

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

* Qt (required >= 5.15)

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

Win:

    perl Configure VC-WIN64A
    nmake

Mac:

    ./Configure darwin64-x86_64-cc no-shared
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

    ninja

How to Install
-----------------
**Linux**
The easies way to install Gittyup is by using [Flatpak](https://www.flatpak.org/)
Ubuntu: apt-get install flatpak flatpak-builder

`flatpak remote-add flathub https://flathub.org/repo/flathub.flatpakrepo`

`flatpak install io.github.gitahead.gitahead` (uses the https://github.com/Murmele/Gittyup fork)

then run with `flatpak run io.github.gitahead.GitAhead`


How to Contribute
-----------------

We welcome contributions of all kinds, including bug fixes, new features,
documentation and translations. By contributing, you agree to release
your contributions under the terms of the license.

Contribute by following the typical
[GitHub workflow](https://guides.github.com/introduction/flow/index.html)
for pull requests. Fork the repository and make changes on a new named
branch. Create pull requests against the `master` branch. Follow the
[seven guidelines](https://chris.beams.io/posts/git-commit/) to writing a
great commit message.

License
-------

Gittyup and its predecessor GitAhead are licensed under the MIT license. See LICENSE.md for details.
