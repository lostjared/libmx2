#!/bin/bash
# Build script for macOS with MoltenVK support
# Requires: brew install molten-vk sdl2 sdl2_ttf sdl2_mixer glm libpng zlib

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== libmx2 macOS MoltenVK Build Script ===${NC}"
echo ""

# Check if running on macOS
if [[ "$(uname)" != "Darwin" ]]; then
    echo -e "${RED}Error: This script is intended for macOS only.${NC}"
    exit 1
fi

# Check for Homebrew
if ! command -v brew &> /dev/null; then
    echo -e "${RED}Error: Homebrew is not installed.${NC}"
    echo "Install Homebrew from: https://brew.sh"
    exit 1
fi

echo -e "${YELLOW}Checking required dependencies...${NC}"

# List of required packages
REQUIRED_PACKAGES=(
    "molten-vk"
    "sdl2"
    "sdl2_ttf"
    "sdl2_mixer"
    "glm"
    "libpng"
    "zlib"
)

MISSING_PACKAGES=()

for pkg in "${REQUIRED_PACKAGES[@]}"; do
    if ! brew list "$pkg" &> /dev/null; then
        MISSING_PACKAGES+=("$pkg")
    fi
done

if [ ${#MISSING_PACKAGES[@]} -ne 0 ]; then
    echo -e "${YELLOW}Missing packages: ${MISSING_PACKAGES[*]}${NC}"
    echo "Installing missing packages..."
    brew install "${MISSING_PACKAGES[@]}"
else
    echo -e "${GREEN}All required packages are installed.${NC}"
fi

# Get Homebrew prefix (different for Intel vs Apple Silicon)
HOMEBREW_PREFIX=$(brew --prefix)
echo "Homebrew prefix: $HOMEBREW_PREFIX"

# Get MoltenVK path
MOLTEN_PATH=$(brew --prefix molten-vk)
echo "MoltenVK path: $MOLTEN_PATH"

# Create build directory
BUILD_DIR="build_molten"
echo ""
echo -e "${YELLOW}Creating build directory: ${BUILD_DIR}${NC}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo ""
echo -e "${YELLOW}Configuring with CMake...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DMOLTEN=ON \
    -DVULKAN=OFF \
    -DOPENGL=OFF \
    -DMIXER=ON \
    -DEXAMPLES=ON \
    -DMOLTEN_PATH="$MOLTEN_PATH" \
    -DCMAKE_PREFIX_PATH="$HOMEBREW_PREFIX" \
    -DCMAKE_INSTALL_PREFIX="$HOMEBREW_PREFIX"

# Build
echo ""
echo -e "${YELLOW}Building...${NC}"
cmake --build . -j$(sysctl -n hw.ncpu)

echo ""
echo -e "${GREEN}=== Build Complete ===${NC}"
echo ""
echo "Executables are in: $(pwd)"
echo ""
echo "To run a Vulkan example:"
echo "  ./vk_skeleton"
echo "  ./vk_matrix"
echo "  ./vk_stars"
echo ""
