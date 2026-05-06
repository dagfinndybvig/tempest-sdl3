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
```bash
./build_native.sh
./tempest
```

### Windows Version (cross-compiled from Linux)
Cross-compile a redistributable Windows x86_64 build using MinGW-w64:
```bash
./build_windows.sh
# Output: dist/windows/{tempest.exe, SDL3.dll, *.wav}
```
The script bootstraps its own dependencies on first run (apt-installs `gcc-mingw-w64-x86-64` and downloads the prebuilt SDL3 MinGW dev package into `third_party/`). Zip up `dist/windows/` for distribution. Targets Windows x86_64; runs on Windows 10 and later. `SDL3.dll` must stay alongside `tempest.exe`.

#### Automated GitHub Releases
Pushing a `v*` tag triggers `.github/workflows/release.yml`, which cross-compiles the Windows build and attaches `tempest-<tag>-windows-x64.zip` to a GitHub Release. To cut a release:
```bash
git tag v0.1.0
git push origin v0.1.0
```
A release can also be triggered manually from the **Actions** tab via the *Release* workflow's *Run workflow* button.

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

### Keyboard Controls (Native & Web)
- **Left / Right Arrow Keys**: Rotate the Blaster around the tunnel rim.
- **Space**: Fire shots down the tunnel.
- **Z**: Activate the Superzapper (once per game - clears all enemies).
- **R**: Reset the game after a Game Over.
- **0–3 Keys**: Switch tunnel geometry while playing. `0` enables the irregular, in-tunnel variant with ±25% angle jitter, `1` selects the circular tunnel, `2` switches to the square tunnel, and `3` activates the dynamic "flat" tunnel with wobble and tilt. During Game Over you can also press any of those keys to restart directly in that geometry.
- **S Key**: (Removed - sound now plays by default)
- **Arrow Up (↑)**: Start game from landing page or restart from game over screen.

### Touch Controls (Web Version)
The game uses **hold-zones with multi-touch** so the player can rotate and fire simultaneously:

- **Left third of screen (hold)**: Rotate counter-clockwise.
- **Right third of screen (hold)**: Rotate clockwise.
- **Bottom-right corner (~25%×25%, tap)**: Activate the Superzapper (one per game).
- **Anywhere else (short tap)**: Fire a shot. Tap is detected on finger-up if the finger moved less than ~25 px and was held under ~350 ms.
- **Landing / Game Over / High-score screens**: Tap anywhere to advance.

Each finger is tracked independently via `SDL_FingerID`, so holding the rotation zone with one thumb while tapping fire with the other works as expected. The on-screen overlay shows the zones (`<<` / `>>` / `ZAP` / `TAP TO FIRE`).

Synthesised mouse events from mobile browsers are suppressed for ~500 ms after any finger event so a single tap doesn't double-trigger.

### Sound Control
- **Sound Control**: Sound effects now play by default (users can control volume through system/browser settings)

## License
MIT License - feel free to experiment with the code!
