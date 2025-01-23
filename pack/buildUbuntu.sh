#!/bin/bash

sudo apt update
sudo apt install -y build-essential libgl1-mesa-dev cmake libgit2-dev cmark git \
                    libssh2-1-dev openssl qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools qttools5-dev ninja-build
cd ..
git fetch
git submodule init
git submodule update
git pull
git checkout deps
cd dep/openssl/openssl/
./config -fPIC
make
cd -
mkdir -vp build/release
cd build/release
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ../..
ninja
