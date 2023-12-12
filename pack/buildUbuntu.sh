sudo apt install build-essential libgl1-mesa-dev
sudo apt install cmake
sudo apt install libgit2-dev
sudo apt install cmark
sudo apt install git
sudo apt install libssh2-1-dev
sudo apt install openssl
sudo apt install qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools
sudo apt install qttools5-dev
sudo apt install ninja-build
cd ../../..
git fetch
git submodule init
git submodule update
git pull
git checkout deps
cd dep/openssl/openssl/
./config -fPIC
make
cd 
mkdir -vp build/release
cd build/release
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ../..
ninja


