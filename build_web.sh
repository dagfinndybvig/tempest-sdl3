#!/bin/bash
# Check if emsdk is available locally first
if [ -f emsdk/emsdk_env.sh ]; then
    echo "Using local emsdk installation..."
    source emsdk/emsdk_env.sh
elif [ -f ~/emsdk/emsdk_env.sh ]; then
    echo "Using system-wide emsdk installation..."
    source ~/emsdk/emsdk_env.sh
else
    echo "ERROR: Emscripten not found. Please install Emscripten first."
    echo "See EMSDK_SETUP.md for installation instructions."
    exit 1
fi

mkdir -p docs
emcc main.c \
    --preload-file laserzap.wav@/ \
    --preload-file explosion.wav@/ \
    --preload-file percussion.wav@/ \
    --preload-file coin.wav@/ \
    --preload-file shotburst.wav@/ \
    -o docs/index.html -sUSE_SDL=3 -sALLOW_MEMORY_GROWTH=1 -lm

if [ $? -eq 0 ]; then
    echo "Web build successful: docs/index.html"
else
    echo "Web build failed."
    exit 1
fi
