#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_BASE="$SCRIPT_DIR"
LIBMX2_REPO="$BUILD_BASE/libmx2"
ACIDDROP_REPO="$BUILD_BASE/AcidDrop"
LIBMX2_DIR="$LIBMX2_REPO/libmx"
ACIDDROP_DIR="$ACIDDROP_REPO"

echo "========================================="
echo "  Full Build: libmx2 + AcidDrop"
echo "========================================="
echo ""
echo "Base directory: $BUILD_BASE"
echo "libmx2 location: $LIBMX2_REPO"
echo "AcidDrop location: $ACIDDROP_REPO"
echo ""

# Clone repos if they don't exist
echo "=== Checking dependencies ==="
if [ ! -d "$LIBMX2_REPO" ]; then
    echo "Cloning libmx2..."
    cd "$BUILD_BASE"
    git clone https://github.com/lostjared/libmx2.git
fi

if [ ! -d "$ACIDDROP_REPO" ]; then
    echo "Cloning AcidDrop..."
    cd "$BUILD_BASE"
    git clone https://github.com/lostjared/AcidDrop.git
fi
echo "✓ Dependencies present"

echo ""
echo "=== Building libmx2 with Vulkan Support ==="
rm -rf "$LIBMX2_DIR/buildvk"
mkdir -p "$LIBMX2_DIR/buildvk"
cd "$LIBMX2_DIR/buildvk"
cmake .. \
    -DEXAMPLES=OFF \
    -DVULKAN=ON \
    -DOPENGL=OFF \
    -DCMAKE_INSTALL_PREFIX=/usr/local/mxvk
make -j$(nproc)
sudo make install
echo "✓ libmx2 built and installed to /usr/local/mxvk"

echo ""
echo "=== Building AcidDrop ==="
rm -rf "$ACIDDROP_DIR/ac"
mkdir -p "$ACIDDROP_DIR/ac"
cd "$ACIDDROP_DIR/ac"
cmake .. \
    -DCMAKE_PREFIX_PATH=/usr/local/mxvk \
    -DCMAKE_INSTALL_PREFIX=/usr/local/mxvk
make -j$(nproc)
echo "✓ AcidDrop built successfully"

echo ""
echo "========================================="
echo "  Build Complete"
echo "========================================="
echo ""
echo "AcidDrop executable:"
echo "  $ACIDDROP_DIR/ac/AcidDrop"
echo ""
echo "To run AcidDrop:"
echo "  $ACIDDROP_DIR/ac/AcidDrop -p $ACIDDROP_DIR"
echo ""

# Ask if user wants to run it now
read -p "Run AcidDrop now? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    cd "$ACIDDROP_DIR/ac"
    ./AcidDrop -p "$ACIDDROP_DIR"
fi
