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
- **Sound Effects**: Five sound effects (laserzap, explosion, percussion, coin, shotburst) with toggle functionality.

## How to Build and Run

### Native Version (Linux)
```bash
./build_native.sh
./tempest
```

### Web Version (Local Testing)
```bash
./build_web.sh
# Then serve the docs directory:
cd docs && python3 -m http.server 8000
# Open browser to http://localhost:8000
```

### Web Version (GitHub Pages Deployment)
The web version is automatically built to the `docs/` directory. Simply push to GitHub and enable GitHub Pages to serve from the `docs` folder.

## Controls
- **Left / Right Arrow Keys**: Rotate the Blaster around the tunnel rim.
- **Space**: Fire shots down the tunnel.
- **Z**: Activate the Superzapper (once per game - clears all enemies).
- **R**: Reset the game after a Game Over.
- **0–3 Keys**: Switch tunnel geometry while playing. `0` enables the irregular, in-tunnel variant with ±25% angle jitter, `1` selects the circular tunnel, `2` switches to the square tunnel, and `3` activates the dynamic "flat" tunnel with wobble and tilt. During Game Over you can also press any of those keys to restart directly in that geometry.
- After you lose all lives, the tunnel randomly switches to one of the four geometries so each Game Over screen feels different.
- On Game Over the background dims and a `PRESS ANY KEY` prompt is drawn in vector lines; press any number key or `R` to restart immediately in that geometry.
- **Sound Toggle**: Click anywhere on the screen to mute/unmute all sound effects (indicator shown at bottom of screen).

## License
MIT License - feel free to experiment with the code!
