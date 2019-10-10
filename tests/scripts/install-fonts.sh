#!/bin/bash
# Installs the bundled fonts with the fontconfig
mkdir ~/.fonts
cp resources/fonts/* ~/.fonts/
fc-cache -v