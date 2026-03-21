# Emscripten Setup Guide for Tempest SDL3

This guide explains how to set up Emscripten for building the WebAssembly version of Tempest SDL3.

## Option 1: Quick Setup (Recommended)

The easiest way to get started is to install Emscripten system-wide:

```bash
# Install Emscripten system-wide (Linux/macOS)
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
git pull
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
cd ..
```

Then you can build the web version:
```bash
./build_web.sh
```

## Option 2: Local Emscripten Installation

If you prefer to keep Emscripten local to this project:

```bash
# Clone emsdk into this directory
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
git pull
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
cd ..
```

## Build Scripts

The repository includes two build scripts:

- `build_native.sh` - Builds the native Linux version
- `build_web.sh` - Builds the WebAssembly version (requires Emscripten)

## Requirements

- **Native build**: GCC, SDL3 development libraries
- **Web build**: Emscripten (emsdk), Python 3

## Troubleshooting

If you get "emcc not found" errors, make sure to:
1. Install Emscripten as shown above
2. Source the environment: `source emsdk/emsdk_env.sh`
3. Try building again

## Docker Alternative

For a completely isolated build environment, consider using the official Emscripten Docker image:

```bash
docker run --rm -v $(pwd):/src -u $(id -u):$(id -g) emscripten/emsdk ./build_web.sh
```
