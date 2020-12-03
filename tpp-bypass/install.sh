#!/bin/bash
# Builds the bypass and installs it in the current user's binary directory
g++ -o tpp-bypass -O2 -std=c++17 -m64 main_bypass.cpp -pthread -lutil
mkdir -p ~/.local/bin
cp ./tpp-bypass ~/.local/bin/tpp-bypass