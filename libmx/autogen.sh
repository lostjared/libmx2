#!/bin/bash
# ==========================================================================
#  autogen.sh – Bootstrap the autotools build system for libmx2
#
#  Usage:
#      ./autogen.sh          # generate configure, Makefile.in, etc.
#      ./configure [options]  # detect deps and create Makefiles
#      make                   # build
#      make install           # install (may need sudo)
# ==========================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "============================================"
echo "  libmx2 – Autotools Bootstrap"
echo "============================================"
echo ""

# ── Check required tools ────────────────────────────────────────────────────
missing=""
for tool in autoreconf aclocal automake autoconf libtoolize pkg-config; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        missing="$missing $tool"
    fi
done

if [ -n "$missing" ]; then
    echo "ERROR: The following required tools are missing:$missing"
    echo ""
    echo "Install them with:"
    echo "  Arch:          sudo pacman -S autoconf automake libtool pkg-config"
    echo "  Debian/Ubuntu: sudo apt install autoconf automake libtool pkg-config"
    echo "  Fedora:        sudo dnf install autoconf automake libtool pkgconfig"
    echo "  macOS:         brew install autoconf automake libtool pkg-config"
    exit 1
fi

# ── Create required directories ─────────────────────────────────────────────
mkdir -p m4 build-aux

# ── Run autoreconf ──────────────────────────────────────────────────────────
echo "Running autoreconf --install --force --verbose ..."
echo ""
autoreconf --install --force --verbose

echo ""
echo "============================================"
echo "  Bootstrap complete!"
echo "============================================"
echo ""
echo "You can now configure and build libmx2:"
echo ""
echo "  ./configure [OPTIONS]"
echo "  make"
echo "  sudo make install"
echo ""
echo "Common configure options:"
echo ""
echo "  --prefix=/usr/local         Install prefix"
echo "  --enable-opengl             OpenGL support          (default: yes)"
echo "  --enable-vulkan             Vulkan support          (default: no)"
echo "  --enable-mixer              SDL2_mixer sound        (default: yes)"
echo "  --enable-jpeg               JPEG image support      (default: no)"
echo "  --enable-opencv             OpenCV support          (default: no)"
echo "  --enable-es3                OpenGL ES3 support      (default: no)"
echo "  --enable-examples           Build example programs  (default: yes)"
echo "  --enable-cross              Cross GL/VK examples    (default: no)"
echo "  --enable-debug              Debug build             (default: no)"
echo ""
echo "  --enable-shared             Build shared libraries  (default: yes)"
echo "  --enable-static             Build static libraries  (default: yes)"
echo ""
echo "Example:"
echo "  ./configure --prefix=/usr --enable-vulkan --enable-jpeg"
echo "  make -j\$(nproc)"
echo "  sudo make install"
echo ""
