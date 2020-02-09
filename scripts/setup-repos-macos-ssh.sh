#!/bin/bash
# clone the additional repositories via ssh, for macOS there is no point in building the tpp-bypass target as it is only useful for WSL
git clone git@github.com:terminalpp/website.git
git clone git@github.com:terminalpp/ropen.git
git clone git@github.com:terminalpp/benchmarks.git
