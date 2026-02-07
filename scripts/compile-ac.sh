#!/bin/bash
set -e

# Use current working directory for builds
BUILD_DIR="acid_build"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "=== Building libmx2 with Vulkan Support ==="
echo "Build directory: $(pwd)"
if [ ! -d "libmx2" ]; then
    git clone https://github.com/lostjared/libmx2.git
fi

cd libmx2/libmx
rm -rf buildvk
mkdir buildvk
cd buildvk
cmake .. -DEXAMPLES=OFF -DVULKAN=ON -DOPENGL=OFF -DCMAKE_INSTALL_PREFIX=/usr/local/mxvk
make -j$(nproc)
sudo make install
echo "✓ libmx2 built and installed"

cd ../../..

echo ""
echo "=== Building AcidDrop ==="
if [ ! -d "AcidDrop" ]; then
    git clone https://github.com/lostjared/AcidDrop.git
fi

cd AcidDrop
rm -rf ac
mkdir ac
cd ac
cmake .. -DCMAKE_PREFIX_PATH=/usr/local/mxvk -DCMAKE_INSTALL_PREFIX=/usr/local/mxvk
make -j$(nproc)
echo "✓ AcidDrop built successfully"

echo ""
echo "=== Build Complete ==="
echo "AcidDrop executable: $(pwd)/AcidDrop"
echo ""
echo "To run AcidDrop:"
echo "  ./AcidDrop -p .."
