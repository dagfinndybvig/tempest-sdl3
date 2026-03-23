#!/bin/bash
# Test script for running Tempest SDL2 in headless environments

echo "Tempest SDL2 Headless Test Script"
echo "=================================="
echo

echo "Option 1: Run with debug output (shows game is working)"
echo "Option 2: Run with Xvfb virtual display (creates virtual screen)"
echo "Option 3: Check if display is available"
echo

read -p "Choose an option (1-3): " choice
echo

case $choice in
    1)
        echo "Running with debug output..."
        echo "You should see window/renderer creation and frame count output."
        echo "Press Ctrl+C to stop."
        echo
        timeout 5 ./tempest_sdl2
        ;;
    
    2)
        echo "Running with Xvfb virtual display..."
        
        # Check if Xvfb is installed
        if ! command -v Xvfb &> /dev/null; then
            echo "Xvfb not found. Installing..."
            sudo apt update && sudo apt install -y xvfb
        fi
        
        # Start virtual display
        echo "Starting virtual display..."
        Xvfb :1 -screen 0 1024x768x24 &
        export DISPLAY=:1
        sleep 1
        
        echo "Running game on virtual display..."
        echo "Note: You won't see the window, but the game is running."
        echo "Press Ctrl+C to stop."
        echo
        timeout 5 ./tempest_sdl2
        
        # Clean up
        pkill Xvfb
        ;;
    
    3)
        echo "Checking display availability..."
        if [ -n "$DISPLAY" ]; then
            echo "DISPLAY is set to: $DISPLAY"
            if command -v xdpyinfo &> /dev/null; then
                xdpyinfo | head -10
            else
                echo "xdpyinfo not available, but DISPLAY is set"
            fi
        else
            echo "No DISPLAY variable set - running headless"
            echo "Use option 2 to test with virtual display"
        fi
        ;;
    
    *)
        echo "Invalid option"
        ;;
esac

echo
echo "Test completed."