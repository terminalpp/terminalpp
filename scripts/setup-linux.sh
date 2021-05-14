#!/bin/bash
sudo apt update

# install the prerequisite packages
sudo apt install cmake libx11-dev libxft-dev libxcursor-dev g++-10

# install qt
sudo apt install libgl1-mesa-dev 
# TODO install QT via aqrtinstaller so that it does not have to be done in actions

# install dev dependencies
sudo apt install imagemagick cloc doxygen graphviz dh-make devscripts rpm


