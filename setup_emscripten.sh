#!/bin/bash

# Script to install Emscripten locally (no sudo required)
# Run this script from the repository root.

echo "Setting up Emscripten..."

# Clone the Emscripten SDK
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk || exit 1

# Install the latest tools
echo "Updating and installing latest Emscripten tools..."
./emsdk update
./emsdk install latest

# Activate the installation
echo "Activating Emscripten..."
./emsdk activate latest

# Set environment variables
echo "Setting environment variables..."
source ./emsdk_env.sh

# Verify the installation
echo "Verifying Emscripten installation..."
emcc --version

if [ $? -eq 0 ]; then
    echo "Emscripten installed successfully!"
    echo "To use Emscripten in a new terminal session, run:"
    echo "  source /path/to/emsdk/emsdk_env.sh"
else
    echo "Emscripten installation failed."
    exit 1
fi

# Return to the repository root
cd ..

echo "Setup complete. You can now run ./build_web.sh to rebuild the web artifacts."