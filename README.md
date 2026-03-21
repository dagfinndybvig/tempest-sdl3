<img width="798" height="663" alt="image" src="https://github.com/user-attachments/assets/de9f8506-c2b0-471f-9f4b-1974c0c3805b" />



# Tempest SDL3 Prototype

An experimental 3D vector-style engine built with C and SDL3, compiled to WebAssembly via Emscripten.

## ⚠️ Project Status
**This is an experimental project inspired by the classic arcade game *Tempest*.** It is **not** a direct port or a recreation of the original game code. It aims to capture the "Vector Glow" aesthetic and the unique 3D tunnel perspective using modern hardware-accelerated rendering.

## Features
- **3D Perspective Tunnel**: A procedurally generated 16-segment tunnel with receding rings.
- **Hardware Accelerated Lines**: Uses SDL3's new rendering API for high-performance vector graphics.
- **WebAssembly Ready**: Fully compatible with modern browsers using Emscripten.
- **Retro Aesthetic**: Neon-on-black color palette inspired by 1980s vector monitors.
- **Gameplay Systems**: Scoring, player lives, Superzapper, and Game Over states.
- **Custom Vector HUD**: Retro-style digit rendering for score and status.
- **Persistent Highscores**: Native version saves to `highscores.txt`, web version uses browser localStorage.
- **Sound Effects**: Five sound effects (laserzap, explosion, percussion, coin, shotburst) that play by default.
- **Touch Controls (Web)**: Experimental touch controls for mobile devices (see notes below).

## How to Build and Run

### Native Version (Linux)
The native version requires SDL3, which may need to be built from source:
```bash
./build_native.sh
./tempest
```

**Note:** If you get SDL3 not found errors, see `SDL3_BUILD.md` for instructions on building SDL3 from source.

### Web Version (Local Testing)
First, install Emscripten (see EMSDK_SETUP.md for detailed instructions):
```bash
./build_web.sh
# Then serve the docs directory:
cd docs && python3 -m http.server 8000
# Open browser to http://localhost:8000
```

### Web Version (GitHub Pages Deployment)
The web version is automatically built to the `docs/` directory. Simply push to GitHub and enable GitHub Pages to serve from the `docs` folder.

## Controls

### Keyboard Controls (Native & Web)
- **Left / Right Arrow Keys**: Rotate the Blaster around the tunnel rim.
- **Space**: Fire shots down the tunnel.
- **Z**: Activate the Superzapper (once per game - clears all enemies).
- **R**: Reset the game after a Game Over.
- **0–3 Keys**: Switch tunnel geometry while playing. `0` enables the irregular, in-tunnel variant with ±25% angle jitter, `1` selects the circular tunnel, `2` switches to the square tunnel, and `3` activates the dynamic "flat" tunnel with wobble and tilt. During Game Over you can also press any of those keys to restart directly in that geometry.
- **S Key**: (Removed - sound now plays by default)
- **Arrow Up (↑)**: Start game from landing page or restart from game over screen.

### Touch Controls (Web Version Only) ⚠️
The game features optional touch controls for mobile devices:

**Current Implementation (Simplified):**
- **Left Swipe**: Clockwise rotation (matches left arrow key)
- **Right Swipe**: Counter-clockwise rotation (matches right arrow key)
- **Tap Anywhere**: Fire shots (single shot per tap release)
- **Rotation Speed**: Much slower (20% of original) for better precision

**Activation:**
- **Landing Page**: Tap anywhere to start the game with touch controls active
- **Game Over Screen**: Tap anywhere to restart the game with touch controls active
- **During Gameplay**: Touch controls are always active when enabled

**Status:**
- ⚠️ **Experimental**: Touch controls are implemented but may require further refinement
- ✅ **Fixed**: Web compatibility issues with static variables resolved
- 🔄 **Simplified**: Circular swipe gestures replaced with simpler left/right swipes
- 🐛 **Testing Needed**: Real-world mobile device testing required

**Known Limitations:**
- Rotation speed and swipe sensitivity may need adjustment
- No visual touch indicators in current simplified version
- Touch controls may feel different from keyboard controls

**Previous Complex Implementation:**
The game previously had circular swipe gestures with visual indicators (blue ring, red fire zone, etc.) but this was simplified to improve reliability and fix web compatibility issues.

### Sound Control
- **Sound Control**: Sound effects now play by default (users can control volume through system/browser settings)

## License
MIT License - feel free to experiment with the code!
