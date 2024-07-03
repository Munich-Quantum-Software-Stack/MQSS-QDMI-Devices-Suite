cd build
clear
make
make install
export QDMI_CONFIG_FILE=../.qdmi-config
./QDMIOpenClose
cd ..
