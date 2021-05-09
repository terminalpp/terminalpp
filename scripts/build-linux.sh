#!/bin/bash
# fetch subprojects
bash scripts/setup-repos.sh
# install dependencies
bash scripts/setup-linux.sh
# create build dirs and build the application
mkdir -p build
cd build
mkdir debug
mkdir release
cd debug
cmake ../.. -DCMAKE_BUILD_TYPE=debug -DCMAKE_C_COMPILER=gcc-8 -DCMAKE_CXX_COMPILER=g++-9
cmake --build .
cd ../release
cmake ../.. -DCMAKE_BUILD_TYPE=release -DCMAKE_C_COMPILER=gcc-8 -DCMAKE_CXX_COMPILER=g++-9
cmake --build .

# build the packages
cmake ../.. -DCMAKE_BUILD_TYPE=release -DCMAKE_C_COMPILER=gcc-8 -DCMAKE_CXX_COMPILER=g++-9 -DPACKAGE_INSTALL=terminalpp
cmake --build . --target packages
cmake ../.. -DCMAKE_BUILD_TYPE=release -DCMAKE_C_COMPILER=gcc-8 -DCMAKE_CXX_COMPILER=g++-9 -DPACKAGE_INSTALL=tpp-ropen
cmake --build . --target packages
cmake ../.. -DCMAKE_BUILD_TYPE=release -DCMAKE_C_COMPILER=gcc-8 -DCMAKE_CXX_COMPILER=g++-9 -DPACKAGE_INSTALL=tpp-bypass
cmake --build . --target packages


