#!/bin/bash
source ~/emsdk/emsdk_env.sh
emcc main.c -o index.html -sUSE_SDL=3 -sALLOW_MEMORY_GROWTH=1 -lm
