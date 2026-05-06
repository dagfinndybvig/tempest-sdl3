# Controls & Web Audit — Tempest SDL3

Audit date: 2026-05-06. Focus: input handling on web/touch devices, with platform and adjacent issues that affect playability.

---

## 1. Critical — touch input is broken

### 1.1 Every swipe goes clockwise, regardless of finger direction
`main.c` ~1264–1308. Two `FINGER_DOWN` blocks fire for the same event:

- The first (inside `STATE_PLAYING`) writes `lastTouchX = relX` in **center-relative pixel space**.
- The second, unconditional block at [main.c#L1304](main.c#L1304) overwrites it with `event.tfinger.x * w` — **raw screen pixel space**.

The next `FINGER_MOTION` computes `deltaX = currentX (center-relative) − lastTouchX (raw pixels)`, which is roughly `−w/2` for any starting position near screen center. That always exceeds the 30 px threshold on the negative side, so `touchRightActive = true /* clockwise */` is set, then `isSwiping = true` locks out all further motion until `FINGER_UP`.

**Symptom:** every swipe produces a single clockwise rotation tick. Counter-clockwise rotation is unreachable from touch.

**Fix:** delete the second unconditional `FINGER_DOWN` block; or unify both into one handler that stores coordinates in a single, documented frame.

### 1.2 "Tap to fire" is actually a 5 Hz autofire
`main.c` ~1454–1499. The fire logic lives in the **per-frame update loop**, not in event handlers:

```c
if (!ctx->touchLeftActive && !ctx->touchRightActive && !ctx->isSwiping) {
    if (!ctx->wasTouching) { ctx->wasTouching = true; ctx->fireTriggered = false; }
    else { ctx->wasTouching = false; if (!ctx->fireTriggered) { ... fire ... } }
}
```

`wasTouching` is never driven by `FINGER_DOWN`/`FINGER_UP`. With no finger on screen the predicate is always true, so `wasTouching` flips `false → true → false → …` every frame and fires whenever the 200 ms cooldown elapses. The player is firing five shots per second whether they tap or not.

**Fix:** detect taps inside the event loop. Track `tapStartX/Y` and `tapStartTime` on `FINGER_DOWN`; on `FINGER_UP`, if movement < threshold and duration < ~250 ms and the gesture wasn't classified as a swipe, queue a single shot.

### 1.3 Superzapper is unreachable on touch
`touchSuperzapperActive` is declared in `AppContext`, the activation block in the update loop exists, and the green "ZAP" rectangle is drawn at [main.c#L1063](main.c#L1063). But **no touch handler tests coordinates against that rectangle** — the visual is decorative. Touch players cannot use the Superzapper.

**Fix:** in the `FINGER_DOWN` handler, check `screenX > w*0.7 && screenY > h*0.8` and set `touchSuperzapperActive = true`; clear it on `FINGER_UP`.

### 1.4 CW / CCW HUD labels contradict the swipe directions
[main.c#L1083](main.c#L1083) labels left of the ring "CCW", right of the ring "CW". The swipe code at [main.c#L1287–1294](main.c#L1287-L1294) treats:

- `deltaX < 0` (leftward swipe) → `touchRightActive = true /* clockwise */`
- `deltaX > 0` (rightward swipe) → `touchLeftActive = true /* counter-clockwise */`

The labels and the active variable names also contradict the actual segment math: `touchLeftActive` increments `playerSegment` (which visually moves the blaster *clockwise* around the ring depending on tunnel orientation). Reconcile labels, variable names, and segment direction once.

### 1.5 No mid-gesture reversal
Once `isSwiping = true`, all further motion is ignored until `FINGER_UP`. To reverse, the player must lift the finger and start again — wrong feel for Tempest, where rotation is continuous and reversible.

### 1.6 No simultaneous move + fire
`isSwiping` and `touchLeftActive/Right` short-circuit fire detection. Movement and firing are mutually exclusive.

### 1.7 In-game tap toggles touch UI off
A misplaced tap during gameplay runs `ctx->showTouchControls = !ctx->showTouchControls`. On a phone that is the only input layer; recovery requires a page reload.

**Fix:** remove the toggle, or move it behind an explicit settings affordance.

### 1.8 Landing / game-over screens use `MOUSE_BUTTON_DOWN`, not `FINGER_DOWN`
Works in most browsers via Emscripten's synthesised mouse events but is fragile (iOS Safari has dropped synthesis after canvas focus changes). Handle both, or `FINGER_DOWN` only.

---

## 2. Critical — web platform / page-level

### 2.1 `build_web.sh` overwrites `docs/index.html` on every build
[build_web.sh](build_web.sh) calls `emcc … -o docs/index.html` with no `--shell-file`. Any manual HTML edits (viewport meta, `touch-action`, etc.) are wiped on the next build.

**Fix:** check in a `shell.html` and pass `--shell-file=shell.html` to `emcc`.

### 2.2 Missing `<meta name="viewport">`
[docs/index.html](docs/index.html) has no viewport meta. Mobile browsers lay out at ~980 px logical width; the 800 px canvas renders tiny.

**Fix in shell.html:**
```html
<meta name="viewport" content="width=device-width, initial-scale=1.0, viewport-fit=cover, user-scalable=no">
```

### 2.3 Missing `touch-action: none` on canvas
Without it, the browser intercepts swipes for scroll/zoom before SDL sees them.

**Fix:** `canvas.emscripten { touch-action: none; }`.

### 2.4 `tabindex=-1` blocks keyboard focus on the canvas
After tapping/clicking the canvas it cannot receive key events. Should be `tabindex=0`.

### 2.5 Fixed 800 × 800 window, no resize handling
[main.c#L1849](main.c#L1849): `SDL_CreateWindow("Tempest SDL3", 800, 800, 0)`. No `SDL_WINDOW_RESIZABLE`, no `SDL_EVENT_WINDOW_RESIZED` handler. On a 390 px-wide phone the canvas overflows the viewport.

**Fix:** create with `SDL_WINDOW_RESIZABLE`; on web, use `emscripten_set_resize_callback` (or the Emscripten "resize canvas" mode) and call `SDL_SetWindowSize` to match the CSS size of the canvas.

### 2.6 Desktop boilerplate clutter
The page still shows the Emscripten spinner/logo, a debug `<textarea id="output">`, and Resize/Pointer-Lock/Fullscreen checkboxes. These are noise on desktop and hostile on mobile (they push the canvas off-screen).

### 2.7 `fprintf(stderr, "DEBUG: …")` leaks to the on-page textarea
Emscripten routes stderr into the visible `<textarea>`. Players see `DEBUG: GameOver triggered, …` on every game over. Either remove the debug prints or guard them with `#ifndef NDEBUG`.

---

## 3. High — control-model design

### 3.1 Swipe-to-rotate is the wrong model for Tempest
Tempest needs **continuous, reversible** rotation. Swipes are discrete. Recommended replacement:

- **Hold zones:** left third of canvas → rotate one way while held; right third → the other way. Step cadence ~1 segment per 80–120 ms feels right.
- **Center band** (vertical strip ~30–70 % of width): tap to fire; tap-and-hold for rapid fire if desired.
- **Bottom-right corner:** Superzapper.
- Allow simultaneous touches (track each `fingerId` separately) so the player can rotate and fire at once.

### 3.2 No persistent on-screen UI for touch zones during play
The current overlay shows a swipe ring whose code is broken. Replace with translucent edge bands, a center "FIRE" pill, and a corner "ZAP" badge that match the new model — visible only on touch devices, fadeable after first interaction.

### 3.3 Rotation cadence is too slow
Today: 1 segment per 5 frames at 60 FPS = ~83 ms/segment, full revolution ~1.3 s. Acceptable for hold-zones; far too slow when combined with one-tick-per-swipe.

---

## 4. Medium — keyboard & state machine

### 4.1 Score carries over to the first game after a high-score entry
`FinalizeHighScoreEntry` sets `highscoreEntryPending = true`. The next `ResetGame` does `if (!highscoreEntryPending) ctx->score = 0;` — skipped — and only *afterwards* clears the flag at the end of the function. The new game starts with the previous score still on the HUD until the first kill.

**Fix:** clear `score` unconditionally in `ResetGame`; the `highscoreEntryPending` flag should only gate the *display* path back into `STATE_HIGHSCORE_DISPLAY`, not score reset.

### 4.2 `LoadHighScores` on web always loads defaults
[main.c](main.c) ~772:
```c
char* json = (char*)EM_ASM_INT({
    return localStorage.getItem('tempestHighScores') || '';
});
```
`EM_ASM_INT` returns an `int`. The JS string is coerced to `0`, cast to `char*` (NULL), and the parser branch is dead. Saved scores are never read back; defaults appear every load.

**Fix:** use `EM_ASM_PTR` with `stringToNewUTF8(...)`, or call `emscripten_run_script_string`, and `free()` the returned pointer afterwards.

### 4.3 `static nameInitialized` survives game resets
Inside the highscore render function. After the first qualifying game it stays `true` for the whole session; a second high-score won't reinitialise the name field.

**Fix:** make it a struct field on `AppContext` and reset in `ResetGame`.

### 4.4 Highscore name entry: only `a–z` accepted
[main.c#L1414](main.c#L1414) only handles `event.key.key >= 'a' && <= 'z'`. Digits, hyphen, and space are rejected. SDL3 also reports lowercase keysyms regardless of Shift; this is fine, but consider widening to printable ASCII.

### 4.5 Native build: holding `Left`/`Right` does not auto-repeat smoothly
[main.c#L1359](main.c#L1359) advances `playerSegment` on every `KEY_DOWN`, including OS-generated key repeats. Repeat cadence is OS-dependent (~30 ms on Linux), which is faster than the touch path (5-frame cadence). The two control paths feel different. Consider sampling key state via `SDL_GetKeyboardState` each frame and gating with a frame counter, mirroring touch.

### 4.6 `R` and `Up` reset paths diverge
- `Up` → `ContinueGameWithSelectedGeometry` (keeps `gameOverShape`).
- `R` → sets `state = STATE_PLAYING` directly, **without calling `ResetGame`**. Lives, score, etc. are not reset; the player resumes mid-death state. Almost certainly unintended.

### 4.7 `0`–`3` during gameplay don't reset the game, but during game-over they do
[main.c#L1336](main.c#L1336) (during game-over) calls `RestartWithShape` → `ResetGame`. During play [main.c#L1366](main.c#L1366) only calls `SelectTunnelShape`. Inconsistent but documented; flag for review.

---

## 5. Low — dead code, minor

- **`touchOnlyMode` never set true.** [main.c#L1828](main.c#L1828) hard-codes `false` with a "future work" comment; conditional branches keyed off it are dead code.
- **`SelectTunnelShape` clamps invalid input to `TUNNEL_CIRCLE`** but the only callers already pass valid values.
- **`MAX_HIGHSCORES = 5` but `newHighScoreName[20]`**, with `nameEntryCursorPos < 19` cap. JSON serialisation buffer `char json[512]` could overflow if names ever held quote/backslash characters; sanitise on input.
- **`canvas oncontextmenu="event.preventDefault()"`** is fine, but the same canvas has no `onselectstart` handler — long-press on iOS may still trigger selection; add `user-select: none`.

---

## Priority summary

| Pri | Item | Where |
|---|---|---|
| 🔴 | Swipe coordinate-frame bug — every swipe is clockwise | §1.1 |
| 🔴 | Autofire disguised as tap | §1.2 |
| 🔴 | Superzapper unreachable | §1.3 |
| 🔴 | `build_web.sh` overwrites `docs/index.html` on every build | §2.1 |
| 🔴 | Missing viewport meta + `touch-action: none` | §2.2, §2.3 |
| 🟠 | CW/CCW labels lie about direction | §1.4 |
| 🟠 | Replace swipe model with hold-zones + multi-touch fire | §1.5, §1.6, §3.1 |
| 🟠 | Self-disabling touch UI toggle | §1.7 |
| 🟠 | Fixed 800×800 window, no resize handling | §2.5 |
| 🟡 | `tabindex=-1` blocks keyboard focus | §2.4 |
| 🟡 | Desktop boilerplate / debug textarea on page | §2.6 |
| 🟡 | `fprintf(stderr, "DEBUG: …")` visible to players | §2.7 |
| 🟡 | Score carries over after high-score entry | §4.1 |
| 🟡 | Web `LoadHighScores` always returns defaults | §4.2 |
| 🟡 | `R` reset path skips `ResetGame` | §4.6 |
| 🟢 | `static nameInitialized`, `touchOnlyMode`, name-entry charset, etc. | §4.3, §5 |

---

## Recommended implementation order

1. Fix `build_web.sh` to use `--shell-file=shell.html`; check in `shell.html` with viewport meta, `touch-action: none`, `tabindex=0`, `user-select: none`, and stripped boilerplate.
2. Make the canvas resizable; handle `SDL_EVENT_WINDOW_RESIZED`; size canvas to viewport on web.
3. Rip out the swipe code and the broken `FINGER_DOWN` doubling. Replace with: hold-zones for rotation, tap-anywhere-in-fire-band for shots, corner for Superzapper, multi-touch via per-`fingerId` state.
4. Fix `LoadHighScores` (`EM_ASM_PTR`).
5. Fix `ResetGame` score-reset ordering and the `R` reset path.
6. Strip / gate `DEBUG:` stderr prints.
7. Tidy: reconcile CW/CCW labels, remove `touchOnlyMode` dead code, move `nameInitialized` onto the context, widen name-entry charset.
