The Tempest was a play by W. Shakespeare that featured the wizard Prospero.

# Tempest SDL3 Prototype - Project Status

This document tracks the current implementation status and provides technical guidance for future development of the Tempest SDL3 prototype.

## 🚀 Current Implementation

The project is currently a functional 3D vector-style engine prototype built with C and SDL3. It features:

### 1. Rendering Engine
- **3D Perspective Projection**: A custom `Project` function handles the conversion of 3D coordinates `(x, y, z)` to 2D screen coordinates.
- **Tunnel Geometry**: A procedurally generated 16-segment tunnel with receding rings. The tunnel uses a "neon-on-black" palette with depth-based color fading.
- **Vector Aesthetics**: Uses SDL3's rendering API with additive blending (`SDL_BLENDMODE_ADD`) to simulate the "glow" of classic vector monitors.

### 2. Gameplay Mechanics
- **Discrete Movement**: The player's craft (the "Blaster") snaps to one of the 16 tunnel segments.
- **Controls**: 
  - `Left / Right Arrow`: Rotate the Blaster around the tunnel rim.
  - `Space`: Fire red shots down the tunnel segments.
  - `Z`: Trigger Superzapper (one-time screen clear).
  - `R`: Reset game.
  - `0-3`: Switch tunnel geometry during gameplay.
- **Enemy System**: 
  - Enemies are rendered as green "X" shapes.
  - They spawn at the far end of the tunnel and move toward the player.
  - Basic collision detection: Shots destroy enemies if they are in the same segment and close in `z` distance.
- **Scoring & Lives**:
  - Player starts with 3 lives. Losing a life triggers a "Game Over" when reaching zero.
  - Destroying enemies adds 100 points to the score.
- **Highscore System**:
  - **Persistent Storage**: Native version uses file I/O (`highscores.txt`), web version uses localStorage with JSON.
  - **Top 5 Scores**: Displays only the highest 5 scores in descending order.
  - **Integrated Name Entry**: Players enter names directly in the highscore table.
  - **Default Values**: 500, 400, 300, 200, 100 points with name "PROSPERO".
  - **Visual Design**: Glowing red heading, neon green scores, large font matching landing page style.
  - **Game Flow**: Game over → Highscore display (with name entry) → Game over screen.
- **Progressive Difficulty**:
  - **Dynamic Geometry**: Every 8 enemies destroyed triggers a random tunnel geometry change.
  - **Speed Increase**: Game speed increases by 2% with each enemy kill (capped at 2.5x normal speed).
  - **State Preservation**: Score, lives, sound settings, and speed carry over through geometry changes.
  - **Game Over Reset**: Speed and geometry reset to defaults when game restarts after Game Over.

### 3. User Interface (HUD)
- **Vector Digit Rendering**: A custom line-based digit renderer draws the score and lives in a classic vector style.
- **Indicators**: Visual feedback for remaining lives and Superzapper availability.

### 3. Build & Platform Support
- **Native**: Compiles with GCC on Linux (`-lSDL3 -lm`).
- **WebAssembly**: Ready for Emscripten via `build.sh`.

---

## 🛠 Technical Notes for Future Developers

### Coordinate System
- **Z-Axis**: `z=0` is at the viewer. The tunnel rim where the player sits is currently around `z=2.0`. Higher `z` values are further "into" the screen.
- **Angle**: Segments are calculated using `(2.0 * PI / NUM_SIDES)`. The `playerSegment` (0-15) determines the current angular position.

### Adding New Features

#### New Enemy Types
To add new enemies, extend the `Enemy` struct in `main.c` or create a new struct type. Update the "Update enemies" and "Draw Enemies" loops in `MainLoop`.

#### Level Progression
Currently, the tunnel is static. To implement levels, consider:
- Changing `NUM_SIDES` or `TUNNEL_RADIUS` dynamically.
- Modifying the colors in the "Draw Tunnel" loop based on a `currentLevel` variable.

#### Particle Effects
For explosions when enemies are hit, implement a simple particle system:
1. Create a `Particle` struct with `(x, y, z)` and `velocity`.
2. Spawn a burst of particles when `ctx->enemies[i].active = false` during a collision.
3. Update and draw them in `MainLoop`.

### Optimization Tips
- The current collision detection is $O(N \times M)$ where $N$ is shots and $M$ is enemies. Since both are small (10 and 5), this is fine, but consider spatial partitioning if increasing counts significantly.
- Use `SDL_RenderLines` (plural) for batching line draws if performance becomes a bottleneck on low-end hardware.

---

## 📅 Roadmap / TODO
- [x] Implement player health/lives and "Game Over" state.
- [x] Implement sound effects using SDL_Audio.
- [x] Implement different tunnel shapes (square, "flat" open tracks).
- [x] Add geometry 0 (irregular circle) with ±25% angle jitter.
- [x] Add variable, tilted "flat" tunnel geometry with random wobble.
- [x] Randomize the tunnel geometry when the player hits Game Over.
- [x] Display a "PRESS ANY KEY" prompt when the game freezes at Game Over.
- [x] Implement progressive difficulty system (speed increase + geometry changes).
- [x] Add automatic tunnel geometry changes every 8 enemy kills.
- [x] Implement game speed multiplier that increases with each kill.
- [ ] Add "Spikers" that leave trails behind them.
- [x] Implement the "Superzapper" (screen-clear ability).
- [x] Implement persistent highscore system with file I/O (native) and localStorage (web).
- [x] Add integrated name entry directly in highscore display.
- [x] Update visual design to match landing page style (glowing red heading, neon green scores).
- [x] Set reasonable default high scores (500-100 points with "PROSPERO" name).
