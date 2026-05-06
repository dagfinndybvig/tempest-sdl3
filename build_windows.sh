#!/bin/bash
# Cross-compile a Windows x86_64 build of tempest using MinGW-w64.
#
# This script bootstraps its own dependencies on first run:
#   - apt-installs gcc-mingw-w64-x86-64 if not present
#   - downloads + extracts the prebuilt SDL3 MinGW dev package into third_party/
#
# Output:
#   dist/windows/tempest.exe   -- the game (GUI subsystem)
#   dist/windows/SDL3.dll      -- runtime DLL (must ship alongside .exe)
#   dist/windows/*.wav         -- audio assets
#   dist/windows/highscores.txt is created on first run.
#
# To distribute: zip -r tempest-windows-x64.zip dist/windows

set -e

SDL_VERSION="3.4.8"
SDL_TARBALL="SDL3-devel-${SDL_VERSION}-mingw.tar.gz"
SDL_URL="https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/${SDL_TARBALL}"
SDL_DIR="$(pwd)/third_party/SDL3-${SDL_VERSION}/x86_64-w64-mingw32"

# --- 1. MinGW-w64 toolchain --------------------------------------------------
if ! command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
    echo "[setup] Installing gcc-mingw-w64-x86-64..."
    if command -v apt-get >/dev/null 2>&1; then
        sudo apt-get update -qq
        sudo apt-get install -y gcc-mingw-w64-x86-64
    else
        echo "ERROR: x86_64-w64-mingw32-gcc not found and apt-get is unavailable."
        echo "Install MinGW-w64 manually for your distro and rerun."
        exit 1
    fi
fi

# --- 2. SDL3 MinGW dev package ----------------------------------------------
if [ ! -d "$SDL_DIR" ]; then
    echo "[setup] Downloading SDL3 ${SDL_VERSION} MinGW dev package..."
    mkdir -p third_party
    curl -L --fail -o "third_party/${SDL_TARBALL}" "$SDL_URL"
    tar -C third_party -xzf "third_party/${SDL_TARBALL}"
    rm -f "third_party/${SDL_TARBALL}"
fi

if [ ! -f "$SDL_DIR/lib/libSDL3.dll.a" ] || [ ! -f "$SDL_DIR/bin/SDL3.dll" ]; then
    echo "ERROR: SDL3 MinGW package looks incomplete under $SDL_DIR"
    exit 1
fi

# --- 3. Compile --------------------------------------------------------------
OUT_DIR="dist/windows"
mkdir -p "$OUT_DIR"

# -mwindows: GUI subsystem (no console window). Drop this flag if you want a
# console for stderr/printf during debugging.
x86_64-w64-mingw32-gcc main.c \
    -O2 \
    -I"$SDL_DIR/include" \
    -L"$SDL_DIR/lib" \
    -mwindows \
    -o "$OUT_DIR/tempest.exe" \
    -lSDL3 -lm \
    -static-libgcc

# --- 4. Stage runtime artefacts ---------------------------------------------
cp "$SDL_DIR/bin/SDL3.dll" "$OUT_DIR/"
cp laserzap.wav explosion.wav percussion.wav coin.wav shotburst.wav "$OUT_DIR/"

echo
echo "Windows build successful: $OUT_DIR/tempest.exe"
ls -lh "$OUT_DIR"
