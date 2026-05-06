# Workflow

How to update each build of `tempest-sdl3` and when to ship a new release.

## Builds at a glance

| Target | Source of truth | Update mechanism | Audience |
|--------|-----------------|------------------|----------|
| **Web (GitHub Pages)** | `docs/` directory on `main` | `./build_web.sh` then commit `docs/` | Players who use the online version |
| **Linux native** | `main.c` on `main` | `./build_native.sh` (developer's machine) | Developers / anyone with SDL3 installed |
| **Windows x86_64** | GitHub Actions release artefact | Tag `vX.Y.Z` triggers `.github/workflows/release.yml` | End users on Windows 10+ |

All three builds compile from the **same `main.c`** with platform branches under `#ifdef __EMSCRIPTEN__` / `#ifdef _WIN32`. Don't fork the source.

## Project layout

```
main.c                  -- entire game source; ~1900 lines, no headers
shell.html              -- Emscripten HTML template (viewport, canvas sizing)
*.wav                   -- audio assets, loaded at runtime; native + preloaded into wasm
build_native.sh         -- gcc -lSDL3 -lm -> ./tempest
build_web.sh            -- emcc + --shell-file shell.html -> docs/
build_windows.sh        -- self-bootstrapping mingw cross-compile -> dist/windows/
serve_web.sh            -- local HTTP server for testing the web build
docs/                   -- GitHub Pages publishes from here on every push to main
.github/workflows/
    release.yml         -- v* tag -> windows zip release
README.md               -- user-facing project description + build instructions
WORKFLOW.md             -- this file
status.md               -- historical implementation notes (some out-of-date)
CONTROLS.md             -- touch/web controls audit (the basis for the v0.1.0 rework)
third_party/            -- gitignored; SDL3 mingw SDK fetched by build_windows.sh
dist/                   -- gitignored; build_windows.sh output
```

To add a new audio asset: drop the `.wav` next to `main.c`, register it in the `WavType` enum and `wavFilenames[]` in `main.c`, **and** add a `--preload-file <name>.wav@/` line to `build_web.sh` and a `cp <name>.wav` line to `build_windows.sh`. Forgetting either build script will produce a build that crashes only on that platform.

## Syncing with GitHub

- **Sync regularly.** Commit and push small, working changes as you go. Don't let local work pile up — the dev container is ephemeral and uncommitted work is the easiest thing to lose.
- **Pull before you start** a session, especially if you've been working from another machine or used the GitHub web UI:
  ```bash
  git pull --ff-only
  ```
- **Default workflow is direct commits to `main`.** This project is small and the maintainer is solo, so PR ceremony isn't required for routine fixes and tweaks.
- **Use a feature branch when the change is risky.** Examples of "risky":
  - Large refactors that may not compile or run correctly for several commits.
  - Experimental features you might want to abandon.
  - Save-format or control-scheme changes that could regress players' data or muscle memory.
  - Anything you'd want to test for several sessions before publishing.

  ```bash
  git switch -c feature/<short-name>
  # ... commit + push as usual ...
  git push -u origin feature/<short-name>
  # When you're happy, merge back:
  git switch main
  git merge --no-ff feature/<short-name>
  git push
  git branch -d feature/<short-name>
  git push origin --delete feature/<short-name>
  ```

  GitHub Pages serves `docs/` from `main`, and the Release workflow only triggers on `v*` tags pushed to `main`, so feature-branch work doesn't reach end users until you merge.

## Standard change loop

1. Edit `main.c` (or related source).
2. Verify the web build compiles:
   ```bash
   ./build_web.sh
   ```
   Output: `docs/index.{html,js,wasm,data}`. Then test it locally:
   ```bash
   ./serve_web.sh   # serves docs/ on http://localhost:8000
   ```
3. Verify the Windows cross-compile:
   ```bash
   ./build_windows.sh
   ```
   Output: `dist/windows/{tempest.exe, SDL3.dll, *.wav}`.
4. (Optional, if you have SDL3 dev headers locally) verify the Linux native build:
   ```bash
   ./build_native.sh
   ```
5. Commit. **The web build is updated by committing the regenerated `docs/`** — GitHub Pages serves that directory directly from `main`, so `git push` is the deploy.
   ```bash
   git add -A
   git commit -m "<concise message>"
   git push
   ```

## Pre-release smoke test

There are no automated tests. Before tagging a Windows release, manually verify in a fresh build:

- [ ] Landing screen draws; ↑ starts the game (keyboard) and tap-anywhere starts (touch on web).
- [ ] Tunnel renders; ←/→ rotate; Space fires; shots destroy enemies.
- [ ] `Z` triggers Superzapper exactly once per game.
- [ ] Keys `0`–`3` switch tunnel geometry mid-game.
- [ ] Death decrements lives; reaching 0 lives shows Game Over.
- [ ] Score qualifies for high score table; name entry works; saved score persists across restart of the app.
- [ ] **After saving one high score, a *second* high score in the same session also qualifies and saves** — this is the v0.1.1 regression and is easy to retest by playing twice in a row.
- [ ] `R` and ↑ on game-over both lead to a clean fresh game (score = 0, lives = 3, no rotation drift).
- [ ] On web: hold-zone overlay shows during play; rotate-and-fire simultaneously works on a touch device.
- [ ] On Windows: `tempest.exe` runs; window is resizable; `highscores.txt` is created next to the exe.

## When to cut a Windows release

**Issue a new tagged Windows release on:**
- Any **major bug fix** that affects gameplay, controls, save data, or stability (e.g. the high-score regression in `v0.1.1`).
- **Completion of a new feature** (new enemy type, new tunnel geometry, audio change, control overhaul, etc.).

Web users get every change automatically via the `docs/` push. Windows users only get what's tagged. **Don't make Windows users wait several fixes for a release.** When in doubt, tag.

## Cutting a release

Use semantic versioning: `vMAJOR.MINOR.PATCH`.

- `PATCH` — bug fixes only, no new features (`v0.1.0` → `v0.1.1`).
- `MINOR` — new features, backward-compatible (`v0.1.x` → `v0.2.0`).
- `MAJOR` — breaking changes to save format, controls layout, etc. (`v0.x.y` → `v1.0.0`).

Steps:

```bash
# 1. Make sure main is clean and pushed.
git status
git pull --ff-only

# 2. Tag and push the tag.
git tag v0.2.0
git push origin v0.2.0
```

Pushing the tag triggers `.github/workflows/release.yml`:

1. Checks out the tagged commit on `ubuntu-24.04`.
2. Runs `./build_windows.sh`, which bootstraps MinGW-w64 and the SDL3 MinGW dev package on the runner.
3. Stages `tempest.exe`, `SDL3.dll`, the WAVs, and `README.md` into `tempest-<tag>-windows-x64.zip`.
4. Creates a GitHub Release at `https://github.com/dagfinndybvig/tempest-sdl3/releases/tag/<tag>` with auto-generated notes and the zip attached.

Watch the run with:
```bash
gh run watch $(gh run list --workflow=release.yml --limit 1 --json databaseId --jq '.[0].databaseId') --exit-status
gh release view v0.2.0
```

A release can also be triggered from the **Actions** tab via *Release → Run workflow*.

## End-user experience

- **Web**: open https://dagfinndybvig.github.io/tempest-sdl3/ (or wherever Pages is served). The page reloads with the new build automatically.
- **Windows**: download `tempest-<tag>-windows-x64.zip` from the Releases page, extract, double-click `tempest.exe`. Windows SmartScreen will warn on the unsigned executable — *More info → Run anyway*. Highscores save to `highscores.txt` next to the exe.

## What's *not* automated

- The native Linux build is not in CI; SDL3 isn't packaged in apt yet on Ubuntu 24.04 in this dev container.
- macOS / Linux binaries are not built or distributed — only Windows.
- Code signing for the Windows binary is not set up; SmartScreen prompts users on first run.
- `CHANGELOG.md` doesn't exist; release notes are auto-generated from PR titles, so direct `main` commits won't show up there. If richer notes are needed, edit the release on GitHub after publishing or maintain a `CHANGELOG.md`.

## Gotchas the codebase has actually been bitten by

Real bugs that have shipped at some point. Treat as a "don't do this again" list.

- **`static` locals inside `MainLoop()` are unsafe under Emscripten.** `emscripten_set_main_loop` re-enters; static state can desync from gameplay state on tab focus changes / browser reflows. Put per-frame state on `AppContext` instead.
- **`EM_ASM_INT` truncates JS heap pointers to 32 bits.** Every wasm pointer is a 64-bit JS number. Use `EM_ASM_PTR` + `stringToNewUTF8` (and remember to `free()` the result on the C side).
- **`emcc -o docs/index.html` overwrites `index.html` with the default Emscripten template every build** unless you pass `--shell-file shell.html`. We rely on the custom shell for viewport meta and `touch-action: none`; missing it silently breaks all mobile input.
- **`AppContext ctx = {0}` leaves persistent fields zeroed if the load path is partial.** `LoadHighScores` must seed sane defaults *before* parsing, then validate every slot afterward — partial JSON / truncated files would otherwise render `     0` rows with empty names.
- **Don't gate `AddHighScore` on a sticky flag.** Any flag set by `FinalizeHighScoreEntry` and only cleared by `ResetGame` will silently drop scores after the first one once players pick "continue with selected geometry" instead of "reset". Score insertion logic must be unconditional.
- **Touch coordinates are normalised (0..1), not pixels.** `event.tfinger.{x,y}` are fractions of the canvas. Multiply by current `SDL_GetWindowSize` each event — don't cache pixel sizes from window-creation time, the canvas is resizable.
- **Mobile browsers synthesise mouse events from touches.** Without explicit suppression (`lastFingerTick` window of ~500 ms), every tap fires twice — once as `FINGER_*`, once as `MOUSE_BUTTON_*`.
- **Native `highscores.txt` is raw `fwrite` of `HighScoreEntry`.** Not portable across CPU/struct-alignment boundaries; not text-editable; will silently break if the struct layout changes. Adding fields to `HighScoreEntry` requires deleting old `highscores.txt` files.

## Updating SDL3

The SDL3 version is pinned in `build_windows.sh` (`SDL_VERSION="3.4.8"`). To bump:

1. Find the latest release on https://github.com/libsdl-org/SDL/releases that has an `SDL3-devel-*-mingw.tar.gz` asset.
2. Edit `SDL_VERSION` in `build_windows.sh`.
3. Delete the cached SDK so the script re-downloads:
   ```bash
   rm -rf third_party
   ./build_windows.sh
   ```
4. Smoke-test the resulting `tempest.exe` on Windows before tagging a release — SDL minor versions occasionally regress audio or input behaviour.

The web build uses Emscripten's bundled SDL3 port (`-sUSE_SDL=3`); to bump that, update emsdk on the build machine. The CI runner installs emsdk fresh per release, so it tracks `latest` automatically.

## Rolling back a bad release

If a tagged Windows release ships a regression:

1. Fix on `main`, verify, push.
2. Bump the patch version and tag a fresh release (`v0.1.1` → `v0.1.2`). **Don't reuse old tags** — users who already downloaded the broken zip won't auto-update, and re-tagging confuses GitHub Releases.
3. Optionally mark the bad release as a pre-release on GitHub so users find the new one first:
   ```bash
   gh release edit v0.1.0 --prerelease
   ```
4. If a release was tagged in error and *no one* has downloaded it yet, you can delete the tag and the release:
   ```bash
   gh release delete v0.1.0 --yes
   git push origin :refs/tags/v0.1.0
   git tag -d v0.1.0
   ```
   Avoid this once external links exist.

## Quick reference

```bash
# Update web (committed and pushed):
./build_web.sh && git add -A docs/ && git commit -m "..." && git push

# Cut a Windows release:
git tag v0.X.Y && git push origin v0.X.Y

# Watch the release build:
gh run watch $(gh run list --workflow=release.yml --limit 1 --json databaseId --jq '.[0].databaseId')
```
