#!/bin/bash

PORT=8000
DIRECTORY="docs"

if [ ! -d "$DIRECTORY" ]; then
    echo "Error: Directory '$DIRECTORY' not found. Please run ./build_web.sh first."
    exit 1
fi

echo "Starting local server for Tempest SDL3..."
echo "Navigate to: http://localhost:$PORT"
echo "Press Ctrl+C to stop the server."

# Run python server from the docs directory
python3 -m http.server $PORT --directory "$DIRECTORY"
