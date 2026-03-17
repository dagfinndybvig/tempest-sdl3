<img width="805" height="631" alt="image" src="https://github.com/user-attachments/assets/46d9fcba-df3c-46ae-9ef7-5a9cb4154344" />


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

## How to Build
To compile for the web (GitHub Pages):
```bash
./build_web.sh
```

To compile natively (Linux):
```bash
./build_native.sh
```

## Controls
- **Left / Right Arrow Keys**: Rotate the Blaster around the tunnel rim.
- **Space**: Fire shots down the tunnel.
- **Z**: Activate the Superzapper (once per game - clears all enemies).
- **R**: Reset the game after a Game Over.
- **0–3 Keys**: Switch tunnel geometry while playing. `0` enables the irregular, in-tunnel variant with ±25% angle jitter, `1` selects the circular tunnel, `2` switches to the square tunnel, and `3` activates the dynamic "flat" tunnel with wobble and tilt. During Game Over you can also press any of those keys to restart directly in that geometry.
- After you lose all lives, the tunnel randomly switches to one of the four geometries so each Game Over screen feels different.
- On Game Over the background dims and a `PRESS ANY KEY` prompt is drawn in vector lines; press any number key or `R` to restart immediately in that geometry.

## License
MIT License - feel free to experiment with the code!
