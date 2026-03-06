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
# cJSON
git clone https://github.com/DaveGamble/cJSON
cd cJSON
mkdir build
cd build
cmake .. -DENABLE_CJSON_UTILS=On -DENABLE_CJSON_TEST=Off -DCMAKE_INSTALL_PREFIX=/usr
make
make DESTDIR=$pkgdir install
cd ../..
rm -rf cJSON
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
