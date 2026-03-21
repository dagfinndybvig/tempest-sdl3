#!/bin/bash
# Check if SDL3 is available
echo "Checking for SDL3 development libraries..."

if pkg-config --exists sdl3; then
    echo "✓ SDL3 found, building native version..."
    gcc main.c -o tempest $(pkg-config --cflags --libs sdl3) -lm
    if [ $? -eq 0 ]; then
        echo "✓ Native build successful: ./tempest"
    else
        echo "✗ Native build failed."
        exit 1
    fi
else
    echo "✗ SDL3 not found. Native build requires SDL3 development libraries."
    echo ""
    echo "Options:"
    echo "1. Install SDL3 from source (see SDL3_BUILD.md)"
    echo "2. Use the web version: ./build_web.sh"
    echo "3. Install SDL3 system packages (if available for your distro)"
    exit 1
fi
