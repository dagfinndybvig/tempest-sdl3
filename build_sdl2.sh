#!/bin/bash
# Build script for SDL2 version of Tempest

echo "Building Tempest SDL2 version..."

# Check if SDL2 is available
if pkg-config --exists sdl2; then
    echo "✓ SDL2 found, building..."
    gcc main_sdl2.c -o tempest_sdl2 $(pkg-config --cflags --libs sdl2) -lm
    if [ $? -eq 0 ]; then
        echo "✓ SDL2 build successful: ./tempest_sdl2"
        echo "Run with: ./tempest_sdl2"
    else
        echo "✗ SDL2 build failed."
        exit 1
    fi
else
    echo "✗ SDL2 not found. Please install SDL2 development libraries."
    echo "On Ubuntu/Debian: sudo apt install libsdl2-dev"
    exit 1
fi