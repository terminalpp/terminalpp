#!/bin/bash
# Downloads the large benchmarks and prepares the environment for running the benchmarks
mkdir -p data/vtebench
cd data/vtebench
wget https://github.com/terminalpp/benchmarks/releases/download/v0.0/alt-screen-write-100.tar.gz -O alt-screen-write-100.tar.gz
tar -xvf alt-screen-write-100.tar.gz
rm alt-screen-write-100.tar.gz

wget https://github.com/terminalpp/benchmarks/releases/download/v0.0/alt-screen-write-color-100.tar.gz -O alt-screen-write-color-100.tar.gz
tar -xvf alt-screen-write-color-100.tar.gz
rm alt-screen-write-color-100.tar.gz

cd ../..
