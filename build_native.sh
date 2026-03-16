#!/bin/bash
gcc main.c -o tempest -lSDL3 -lm
if [ $? -eq 0 ]; then
    echo "Native build successful: ./tempest"
else
    echo "Native build failed."
    exit 1
fi
