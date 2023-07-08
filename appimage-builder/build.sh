#!/bin/bash

git submodule init
git submodule update

# Install Qt
sudo add-apt-repository ppa:beineri/opt-qt-5.15.2-focal -y
sudo apt-get update -qq
sudo apt-get -y install qt515base qt515tools libgl1-mesa-dev
source /opt/qt*/bin/qt*-env.sh

# Install ninja
sudo pip install ninja

# Build OpenSSL; why does it need a private (self-built) version?
cd dep/openssl/openssl
./config -fPIC
make -j $(npoc)
cd ..

# Build
mkdir -p build/release
cd build/release
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/ ../..
ninja
DESTDIR=./appdir ninja install ; find ./appdir
rm -rf ./appdir/usr/include/

# Deploy dependencies into AppDir
wget -c https://github.com/$(wget -q https://github.com/probonopd/go-appimage/releases/expanded_assets/continuous -O - | grep "appimagetool-.*-x86_64.AppImage" | head -n 1 | cut -d '"' -f 2)
chmod +x appimagetool-*.AppImage
LD_LIBRARY_PATH=/opt/qt515/lib/ ./appimagetool-*.AppImage -s deploy ./appdir/usr/share/applications/*.desktop --appimage-extract-and-run # Bundle EVERYTHING

# Turn AppDir into AppImage
./appimagetool-*.AppImage ./appdir --appimage-extract-and-run # turn AppDir into AppImage

# Two files have been generated, both must be uploaded to GitHub Releases for AppImageUpdate to work
ls -lh Gittyup-*
