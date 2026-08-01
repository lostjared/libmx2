#!/usr/bin/env bash

set -Eeuo pipefail

readonly REPO_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly BUILD_DIR="$REPO_DIR/build/pcons"

show_usage() {
    cat <<'EOF'
Usage: ./build-install-pcons.sh [PCONS_VAR=value ...]

Build and install libmx2 with pcons. By default this builds the shared mx and
mxgl libraries, the Vulkan mx_vk/volk libraries, all SDL/OpenGL/Vulkan
examples, mxmod_compress, public headers, and pkg-config files, then installs
the reusable artifacts under /usr/local.

Examples:
  ./build-install-pcons.sh
  ./build-install-pcons.sh EXAMPLES=0
  ./build-install-pcons.sh VULKAN=0
  ./build-install-pcons.sh OPENGL=0
  ./build-install-pcons.sh STATIC=1 EXAMPLES=0

Environment:
  CC, CXX                  C and C++ compilers (defaults: gcc and g++)
  VARIANT                  release or debug (default: release)
  LIBMX2_INSTALL_PREFIX    installation prefix (default: /usr/local)
  LIBMX2_DESTDIR           optional packaging root prepended to the prefix
  LIBMX2_JOBS              optional parallel build job count
  LIBMX2_PCONS             pcons executable (default: pcons)
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    show_usage
    exit 0
fi

compiler_c="${CC:-gcc}"
compiler_cxx="${CXX:-g++}"
variant="${VARIANT:-release}"
pcons_command="${LIBMX2_PCONS:-pcons}"
install_prefix="${LIBMX2_INSTALL_PREFIX:-/usr/local}"
destination_root="${LIBMX2_DESTDIR:-}"
jobs="${LIBMX2_JOBS:-}"
vulkan_option="VULKAN=1"

for argument in "$@"; do
    if [[ "$argument" == VARIANT=* ]]; then
        variant="${argument#VARIANT=}"
    fi
    if [[ "$argument" == VULKAN=* ]]; then
        vulkan_option=""
    fi
    case "$argument" in
        MOLTEN=1 | MOLTEN=on | MOLTEN=true | MOLTEN=yes)
            vulkan_option=""
            ;;
    esac
done

if [[ "$install_prefix" != /* ]]; then
    echo "LIBMX2_INSTALL_PREFIX must be an absolute path: $install_prefix" >&2
    exit 2
fi
if [[ -n "$destination_root" && "$destination_root" != /* ]]; then
    echo "LIBMX2_DESTDIR must be an absolute path: $destination_root" >&2
    exit 2
fi
if [[ -n "$jobs" && ! "$jobs" =~ ^[1-9][0-9]*$ ]]; then
    echo "LIBMX2_JOBS must be a positive integer: $jobs" >&2
    exit 2
fi

for command_name in "$compiler_c" "$compiler_cxx" "$pcons_command" install cp mktemp; do
    if ! command -v "$command_name" >/dev/null 2>&1; then
        echo "Required command not found: $command_name" >&2
        exit 3
    fi
done

install_prefix="${install_prefix%/}"
readonly INSTALL_ROOT="${destination_root}${install_prefix}"

mkdir -p "$BUILD_DIR"
stage_dir="$(mktemp -d "$BUILD_DIR/install-stage.XXXXXX")"
cleanup_stage() {
    if [[ "$stage_dir" == "$BUILD_DIR"/install-stage.* ]]; then
        rm -rf -- "$stage_dir"
    fi
}
trap cleanup_stage EXIT

pcons_options=(-B "$BUILD_DIR" --reconfigure)
if [[ -n "$jobs" ]]; then
    pcons_options+=(-j "$jobs")
fi

echo "Building and staging libmx2..."
feature_defaults=()
if [[ -n "$vulkan_option" ]]; then
    feature_defaults+=("$vulkan_option")
fi
(
    cd "$REPO_DIR"
    CC="$compiler_c" CXX="$compiler_cxx" \
        "$pcons_command" "${pcons_options[@]}" \
        "VARIANT=$variant" \
        "PCONS_INSTALL_PREFIX=$stage_dir" \
        "PCONS_FINAL_PREFIX=$install_prefix" \
        "${feature_defaults[@]}" "$@" all install
)

required_files=(
    "$stage_dir/include/mx2/mx.hpp"
    "$stage_dir/include/mx2/config.h"
    "$stage_dir/lib/pkgconfig/libmx.pc"
    "$stage_dir/bin/mxmod_compress"
)
for required_file in "${required_files[@]}"; do
    if [[ ! -f "$required_file" ]]; then
        echo "Expected staged file was not produced: $required_file" >&2
        exit 4
    fi
done

if ! compgen -G "$stage_dir/lib/libmx.*" >/dev/null; then
    echo "Expected staged mx library was not produced." >&2
    exit 4
fi

install_parent="$INSTALL_ROOT"
while [[ ! -e "$install_parent" && "$install_parent" != "/" ]]; do
    install_parent="$(dirname -- "$install_parent")"
done

privilege_command=()
if ((EUID != 0)) && [[ ! -w "$install_parent" ]]; then
    if ! command -v sudo >/dev/null 2>&1; then
        echo "Installing to $INSTALL_ROOT requires root privileges, but sudo was not found." >&2
        exit 5
    fi
    privilege_command=(sudo)
fi

run_install() {
    "${privilege_command[@]}" "$@"
}

echo "Installing into $INSTALL_ROOT..."
run_install install -d "$INSTALL_ROOT"
run_install cp -a "$stage_dir/." "$INSTALL_ROOT/"

if [[ -z "$destination_root" && ("$install_prefix" == "/usr" || "$install_prefix" == "/usr/local") ]]; then
    if command -v ldconfig >/dev/null 2>&1; then
        run_install ldconfig
    fi
fi

echo
echo "Installation complete."
echo "  Headers:    $INSTALL_ROOT/include/mx2"
echo "  Libraries:  $INSTALL_ROOT/lib"
echo "  pkg-config: $INSTALL_ROOT/lib/pkgconfig"
echo "  Tool:       $INSTALL_ROOT/bin/mxmod_compress"
