#!/bin/bash

git clone https://github.com/lostjared/libmx2.git
cd libmx2/libmx
mkdir buildvk
cd buildvk
cmake .. -DEXAMPLES=OFF -DVULKAN=ON -DOPENGL=OFF -DCMAKE_INSTALL_PREFIX=/usr/local/mxvk
cd buildvk
make -j$(nproc)
sudo make install

git clone https://github.com/lostjared/AcidDrop.git
cd AcidDrop
mkdir ac
cd ac
cmake .. -DCMAKE_PREFIX_PATH=/usr/local/mxvk -DCMAKE_INSTALL_PREFIX=/usr/local/mxvk
make -j$(nproc)
./AcidDrop -p ..
