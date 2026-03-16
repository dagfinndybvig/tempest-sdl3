#!/bin/bash
# Ensure emsdk is in path if needed (user-specific, but keeping it for context)
if [ -f ~/emsdk/emsdk_env.sh ]; then
    source ~/emsdk/emsdk_env.sh
fi

mkdir -p docs
emcc main.c --preload-file laserzap.wav@/ -o docs/index.html -sUSE_SDL=3 -sALLOW_MEMORY_GROWTH=1 -lm

if [ $? -eq 0 ]; then
    echo "Web build successful: docs/index.html"
else
    echo "Web build failed."
    exit 1
fi
