#!/bin/bash
brew install cmake 

# install qt
brew install qt5

# to install:
# TODO move this somewhere where it is visible
cmake .. -DCMAKE_PREFIX_PATH=/usr/local/Cellar/qt/5.14.1/