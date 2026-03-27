#!/bin/bash

cd .. 
# doxygen
wget https://www.doxygen.nl/files/doxygen-1.14.0.linux.bin.tar.gz 
tar -xzf doxygen-1.14.0.linux.bin.tar.gz
cd doxygen-1.14.0 
make 
make install 
cd .. 
ln -s /usr/local/bin/doxygen /usr/bin/doxygen 
rm -rf doxzyen* 
# gtest
git clone https://github.com/google/googletest.git
cd googletest
mkdir build
cd build
cmake ..
make 
make install
cd ../..
rm -rf googletest
cd /workspaces/MQSS-QDMI-Devices-Suite
mkdir build
cd build
cmake ..
make
bash
