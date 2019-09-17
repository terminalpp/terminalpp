#!/bin/bash
# install dependencies
sudo apt install libx11-dev libxft-dev
# install dev dependencies
sudo apt install imagemagick
# create build dirs and build the application
mkdir -p build
cd build
mkdir debug
mkdir release
cd debug
cmake ../.. -DCMAKE_BUILD_TYPE=debug -DCMAKE_C_COMPILER=gcc-7 -DCMAKE_CXX_COMPILER=g++-7
cmake --build .
cd ../release
cmake ../.. -DCMAKE_BUILD_TYPE=release -DCMAKE_C_COMPILER=gcc-7 -DCMAKE_CXX_COMPILER=g++-7
cmake --build .
