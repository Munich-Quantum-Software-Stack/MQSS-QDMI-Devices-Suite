cd build
clear
make
make install
export QDMI_CONFIG_FILE=./tests/.qdmi-config
./tests/WMIBackend
cd ..
