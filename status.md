It was also made into a movie by Peter Greenaway, who called it Prospero's Books.

>>>>>>> Stashed changes
=======
The Tempest was a play by W. Shakespeare that featured the wizard Prospero.

It was also made into a movie by Peter Greenaway, who called it Prospero's Books.
=======
It was also made into a movie by Peter Greenaway, who called it Prospero's Books.

>>>>>>> Stashed changes
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
  - **Touch Controls (Web only)**:
    - **Circular Swipe Gestures**: Implemented using SDL3 touch events with vector angle calculation
    - **Swipe Detection Algorithm**:
      - Converts touch coordinates to center-relative position
      - Calculates swipe vector using `atan2f(deltaY, deltaX)` for angle determination
      - Left swipes (45°-135° angles): Counter-clockwise rotation
      - Right swipes (-45° to 45° or 135°-180° angles): Clockwise rotation
      - Minimum 50px distance from center required to register swipe
    - **Tap Controls**:
      - Center area (40-60% width/height): Fire shots
      - Bottom-right corner (70-100% X, 80-100% Y): Superzapper activation
    - **State Management**:
      - `lastTouchX/Y`: Track previous touch position for vector calculation
      - `isSwiping`: Boolean flag to prevent multiple swipe detections per gesture
      - Touch controls toggle via tap during gameplay
    - **Visual Feedback**:
      - Blue circular ring (40% of screen size) indicates swipe area
      - Red inner circle (30% of swipe radius) shows fire zone
      - Directional labels: "CW" (clockwise), "CCW" (counter-clockwise)
      - Green corner rectangle for Superzapper with "ZAP" label
    - **Activation**: Tap anywhere on landing/game over screens to enable touch controls
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
- **Optional Activation**: Touch controls are optional and activated by user choice.
- **Landing Page**: Shows "PRESS UP TO START" and "OR TAP TO START WITH TOUCH CONTROLS".
- **Game Over Screen**: Shows "PRESS UP TO RESTART" and "OR TAP TO RESTART WITH TOUCH CONTROLS".
- **Touch Zones**:
  - Left 30% of screen: Rotate counter-clockwise (continuous)
  - Right 30% of screen: Rotate clockwise (continuous)
  - Bottom center (30-70%): Fire shots (edge-triggered)
  - Bottom right (70-100%): Superzapper (edge-triggered)
- **Visual Feedback**: Semi-transparent colored zones with text labels ("<<<", ">>>", "FIRE", "ZAP").
- **Toggle During Gameplay**: Screen tap toggles touch controls on/off.
- **Conditional Compilation**: Touch code only compiled in web version (`#ifdef __EMSCRIPTEN__`).
- **Sound Control**: S key toggles sound in both native and web versions.

### 3. Build & Platform Support ✅
- **Native**: Compiles with GCC on Linux (`-lSDL3 -lm`).
- **WebAssembly**: Ready for Emscripten via `build.sh`.

### 4. Touch Controls (Web Version) ⚠️

**Status**: Touch controls are implemented but may require further refinement based on user testing.

#### Current Implementation
- **Simplified Controls**: Left/right swipes for rotation, tap anywhere to fire
- **Rotation Direction**: 
  - Left swipe = Clockwise (matches left arrow key)
  - Right swipe = Counter-clockwise (matches right arrow key)
- **Firing**: Single shot per tap release with 200ms cooldown
- **Rotation Speed**: 20% of original speed (much slower for precision)
- **State Management**: Uses context struct variables for web compatibility

#### Known Issues & Limitations
- **Web Compatibility**: Initially used static variables which don't work in WebAssembly
- **Fixed**: Moved all state to AppContext struct for proper persistence
- **Testing Required**: Real-world mobile device testing needed
- **Potential Improvements**: May need adjustment to swipe sensitivity and rotation speed

#### Technical Implementation
- **Swipe Detection**: Horizontal movement only (30px minimum)
- **Tap Detection**: Any tap not detected as swipe triggers firing
- **State Variables**: `rotationFrameCounter`, `wasTouching`, `fireTriggered`, `lastFireTime`
- **Initialization**: Properly reset in `ResetGame()` function

---

## 🛠 Technical Notes for Future Developers

### Touch Controls Implementation

**Important**: Static variables in `MainLoop()` don't work in WebAssembly builds! Always use context struct.

#### Current Working Implementation
- **Swipe Detection**: Left/right horizontal swipes only (simplified from circular)
- **Rotation**: 1 segment every 5 frames (20% speed)
- **Firing**: Edge-triggered on tap release with cooldown
- **State**: All variables in `AppContext` struct for web compatibility

#### Event Handling
- `SDL_EVENT_FINGER_DOWN`: Start tracking touch position
- `SDL_EVENT_FINGER_MOTION`: Detect swipe direction (left/right)
- `SDL_EVENT_FINGER_UP`: Reset swipe state
- **Swipe Logic**: 30px minimum horizontal movement to register

#### State Variables (in AppContext)
```c
int rotationFrameCounter;  // For slow rotation
bool wasTouching;         // Tap detection
bool fireTriggered;       // Prevent multiple fires
Uint64 lastFireTime;      // Cooldown timing
```

#### Initialization
Must be reset in `ResetGame()`:
```c
ctx->rotationFrameCounter = 0;
ctx->wasTouching = false;
ctx->fireTriggered = false;
ctx->lastFireTime = 0;
```

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
