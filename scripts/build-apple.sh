#!/bin/bash
# install dependencies
xcode-select --install
brew install Caskroom/cask/xquartz
brew install fontconfig
# build the app
mkdir -p build
cd build
mkdir debug
mkdir release
cd debug
cmake ../.. -DCMAKE_BUILD_TYPE=debug
cmake --build .
cd ../release
cmake ../.. -DCMAKE_BUILD_TYPE=release
cmake --build .

