#!/bin/bash
# Cross-compile a Windows x86_64 build of tempest using MinGW-w64.
#
# Requirements (already installed in this dev container):
#   apt-get install gcc-mingw-w64-x86-64
#   third_party/SDL3-3.4.8/x86_64-w64-mingw32/{include,lib,bin}
#
# Output:
#   dist/windows/tempest.exe        -- the game
#   dist/windows/SDL3.dll           -- runtime DLL (must ship alongside .exe)
#   dist/windows/*.wav              -- audio assets
#   dist/windows/highscores.txt is created on first run.
#
# To distribute, zip up the dist/windows directory.

set -e

SDL_DIR="$(pwd)/third_party/SDL3-3.4.8/x86_64-w64-mingw32"
if [ ! -d "$SDL_DIR" ]; then
    echo "Missing $SDL_DIR. Download SDL3-devel-*-mingw.tar.gz from"
    echo "https://github.com/libsdl-org/SDL/releases and extract under"
    echo "third_party/."
    exit 1
fi

OUT_DIR="dist/windows"
mkdir -p "$OUT_DIR"

# -mwindows: build a GUI app (no console window pops up). Drop this flag if
# you want a console for stderr/printf during debugging.
x86_64-w64-mingw32-gcc main.c \
    -O2 \
    -I"$SDL_DIR/include" \
    -L"$SDL_DIR/lib" \
    -mwindows \
    -o "$OUT_DIR/tempest.exe" \
    -lSDL3 -lm \
    -static-libgcc

if [ $? -ne 0 ]; then
    echo "Windows build failed."
    exit 1
fi

# Copy runtime artefacts. SDL3.dll must sit next to the .exe.
cp "$SDL_DIR/bin/SDL3.dll" "$OUT_DIR/"
cp laserzap.wav explosion.wav percussion.wav coin.wav shotburst.wav "$OUT_DIR/"

echo "Windows build successful: $OUT_DIR/tempest.exe"
ls -lh "$OUT_DIR"
