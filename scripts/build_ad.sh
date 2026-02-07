#!/bin/bash

# Clone and build libmx2 with Vulkan support
git clone https://github.com/lostjared/libmx2.git
cd libmx2
mkdir build && cd build
cmake -B . -S ../libmx -DVULKAN=ON -DOPENGL=OFF -DCMAKE_INSTALL_PREFIX=/usr/local/mxvk
make -j$(nproc)
sudo make install

# Go back to parent directory and clone AcidDrop
cd ../..
git clone https://github.com/lostjared/AcidDrop.git
cd AcidDrop

# Build AcidDrop
mkdir ac && cd ac
cmake .. -DCMAKE_PREFIX_PATH=/usr/local/mxvk -DCMAKE_INSTALL_PREFIX=/usr/local/mxvk
make -j$(nproc)

# Run AcidDrop (pointing to parent directory for data files)
./AcidDrop -p ..
