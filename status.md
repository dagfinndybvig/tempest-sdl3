The Tempest was a play by W. Shakespeare that featured the wizard Prospero.

It was also made into a movie by Peter Greenaway.

# Tempest SDL3 Prototype - Project Status

This document tracks the current implementation status and provides technical guidance for future development of the Tempest SDL3 prototype.

## 🚀 Current Implementation (as of 2024)

The project is a functional Tempest-inspired 3D vector-style game built with C and SDL3. Current status:

### 1. Core Gameplay ✅
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
  - `S/s`: Toggle sound effects on/off (case-insensitive).
  - **Touch Controls (Web only)**: Hold-zone model with multi-touch (per-finger tracking via `SDL_FingerID`).
    - Left third of screen held = rotate counter-clockwise.
    - Right third of screen held = rotate clockwise.
    - Bottom-right corner (~25%×25%) tap = Superzapper (edge-triggered on finger-down).
    - Short tap anywhere else = fire (edge-triggered on finger-up; <25 px movement, <350 ms).
    - Landing / Game Over / High-score: tap anywhere to advance.
    - Synthesised mouse events suppressed within 500 ms of any finger event.
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

### 4. Touch Controls (Web Version)
- **Hold-zone model** with per-finger tracking (`SDL_FingerID`); multi-touch lets the player rotate and fire simultaneously.
- **Touch Zones**:
  - Left 30% of screen (held): rotate counter-clockwise.
  - Right 30% of screen (held): rotate clockwise.
  - Bottom-right ~25%×25% (tapped): Superzapper, edge-triggered on finger-down.
  - Anywhere else (short tap, <25 px movement, <350 ms): fire, edge-triggered on finger-up.
- **State-screen taps**: Tap anywhere on Landing / Game Over / High-score to advance.
- **Mouse-event suppression**: ~500 ms after any finger event, mouse-button events are ignored to avoid mobile-browser double-fire.
- **Visual overlay**: Hold-zone rectangles labelled `<<` / `>>` / `ZAP` / `TAP TO FIRE`.
- **Conditional Compilation**: Touch UI only drawn in web build (`#ifdef __EMSCRIPTEN__`).
- **Sound Control**: S key toggles sound in both native and web versions.

### 3. Build & Platform Support ✅
- **Native**: Compiles with GCC on Linux (`-lSDL3 -lm`).
- **WebAssembly**: Ready for Emscripten via `build.sh`.

### 4. Touch Controls (Web Version)

**Status**: Reworked from broken swipe model to multi-touch hold-zones. See `CONTROLS.md` for the audit that drove this change.

#### Implementation
- **Per-finger tracking**: `AppContext.fingers[8]` array keyed on `SDL_FingerID`. Each slot stores `id`, `active`, `zone`, `startX/Y`, `startTick`, `moved`.
- **Zone classification** (`ClassifyTouchZone`): runs on `FINGER_DOWN`, frozen for the lifetime of that finger.
  - `1` = left third (CCW), `2` = right third (CW), `3` = bottom-right corner (Superzapper), `0` = neutral/fire.
- **Rotation**: each frame, scan active fingers; if any has zone 1, request CCW; if any has zone 2, request CW. Cadence is throttled to one segment every 3 frames via `ctx->rotationFrameCounter`.
- **Fire**: edge-triggered on `FINGER_UP` if `!moved` and duration <350 ms and zone == 0. Sets `ctx->firePending`, drained per frame in `MainLoop`.
- **Superzapper**: edge-triggered on `FINGER_DOWN` if zone == 3. Sets `ctx->superzapperPending`.
- **Mouse suppression**: a `lastFingerTick` timestamp causes mouse-button-down events within 500 ms of a finger event to be ignored (mobile browsers synthesise mouse events from touches).

#### Event Handling
- `SDL_EVENT_FINGER_DOWN`: allocate slot via `FindOrAllocFinger`, classify zone, fire Superzapper if applicable.
- `SDL_EVENT_FINGER_MOTION`: mark `moved = true` once finger has travelled >25 px from start.
- `SDL_EVENT_FINGER_UP`: queue fire if it qualified as a tap; release the slot.

#### State Variables (in AppContext)
```c
struct {
    SDL_FingerID id;
    bool active;
    int zone;
    float startX, startY;
    Uint64 startTick;
    bool moved;
} fingers[MAX_TOUCH_FINGERS];
int rotationFrameCounter;
bool firePending;
bool superzapperPending;
```

All touch state lives in `AppContext` (no `static` locals in `MainLoop`, which Emscripten/WASM does not handle predictably across `emscripten_set_main_loop` invocations). `ResetGame` clears all finger slots and pending flags.

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
- The current collision detection is $O(N 	imes M)$ where $N$ is shots and $M$ is enemies. Since both are small (10 and 5), this is fine, but consider spatial partitioning if increasing counts significantly.
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
- [x] Add optional touch controls for mobile devices (web version only).
- [x] Implement touch control activation via landing page tap.
- [x] Add visual touch indicators (semi-transparent zones with labels).
- [x] Implement touch zone logic (left/right rotation, fire, superzapper).
- [x] Add touch control toggle during gameplay via screen tap.
- [x] Implement game over screen tap restart with touch controls.
- [x] Add consistent touch control messaging to both landing and game over screens.
- [x] Reorganize controls: S key toggles sound, mouse click toggles touch controls.
- [x] Make S key sound toggle case-insensitive.
- [x] Ensure landing page only starts with Arrow Up key (not any key).
- [x] Replace swipe-based touch controls with multi-touch hold-zone model.
- [x] Custom Emscripten shell (`shell.html`) with viewport meta and `touch-action: none`.
- [x] Fix `LoadHighScores` web bug: `EM_ASM_INT` truncated heap pointer; switch to `EM_ASM_PTR` + `stringToNewUTF8`.
- [x] Reset score unconditionally in `ResetGame` and `ContinueGameWithSelectedGeometry`; `R` in GAMEOVER now calls `ResetGame`.
- [x] Make window resizable (`SDL_WINDOW_RESIZABLE`); layout reads `SDL_GetWindowSize` per frame.
- [x] Strip `DEBUG:` `printf`/`fprintf` spam; widen name-entry charset; replace static `nameInitialized` with `ctx->nameEntryInitialized`.
