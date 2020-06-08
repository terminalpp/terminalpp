#!/bin/bash
sudo apt update

# install the prerequisite packages
sudo apt install cmake libx11-dev libxft-dev g++

# install qt
sudo apt install qt5-default

# install dev dependencies
sudo apt install imagemagick cloc doxygen graphviz dh-make devscripts rpm
