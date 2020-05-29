#!/bin/bash
# requires the packages necessary for the build system to work to be already installed as this runs on self-hosted runner
#
# Debian: cmake libx11-dev libxft-dev g++-8
# Ubuntu: cmake libx11-dev libxft-dev g++-8
# OpenSuSE : cmake libX11-devel libXft-devel gcc8-c++ rpmbuild
echo "Building tpp-bypass for distribution $1"
mkdir -p build-$1
cd build-$1
cmake .. -DCMAKE_BUILD_TYPE=release -DCMAKE_C_COMPILER=gcc-8 -DCMAKE_CXX_COMPILER=g++-8
cmake --build . --target tpp-bypass
