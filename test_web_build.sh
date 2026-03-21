#!/bin/bash
# Simple test script to verify web build works
echo "Testing web build..."

# Check if emsdk is available
if [ -f emsdk/emsdk_env.sh ]; then
    echo "✓ Local emsdk found"
    source emsdk/emsdk_env.sh
elif [ -f ~/emsdk/emsdk_env.sh ]; then
    echo "✓ System emsdk found"
    source ~/emsdk/emsdk_env.sh
else
    echo "✗ Emscripten not found - please install first"
    echo "See EMSDK_SETUP.md for instructions"
    exit 1
fi

# Test the build
echo "Building web version..."
if ./build_web.sh; then
    echo "✓ Web build successful!"
    
    # Check that output files exist
    if [ -f docs/index.html ] && [ -f docs/index.js ] && [ -f docs/index.wasm ]; then
        echo "✓ All output files generated"
        echo ""
        echo "Build successful! You can serve the docs directory:"
        echo "  cd docs && python3 -m http.server 8000"
        echo "  Then open http://localhost:8000 in your browser"
    else
        echo "✗ Missing output files"
        exit 1
    fi
else
    echo "✗ Web build failed"
    exit 1
fi
