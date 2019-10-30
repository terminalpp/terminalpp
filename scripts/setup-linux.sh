#!/bin/bash

# add the extra repositories
git clone https://github.com/terminalpp/website
git clone https://github.com/terminalpp/bypass
git clone https://github.com/terminalpp/ropen

# install the prerequisite packages
sudo apt install cmake libx11-dev libxft-dev

# install dev dependencies
sudo apt install imagemagick
