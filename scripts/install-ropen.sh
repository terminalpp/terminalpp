#!/bin/bash
# fetch subprojects
bash scripts/setup-repos.sh
# install dependencies
bash scripts/setup-linux.sh

# create build dirs and build the application
mkdir -p build/release
cd build/release

# build & install the ropen package
cmake ../.. -DCMAKE_BUILD_TYPE=release -DCMAKE_C_COMPILER=gcc-8 -DCMAKE_CXX_COMPILER=g++-8 -DPACKAGE_INSTALL=tpp-ropen
sudo cmake --build . --target install
