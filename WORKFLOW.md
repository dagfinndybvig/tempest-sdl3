# Workflow

How to update each build of `tempest-sdl3` and when to ship a new release.

## Builds at a glance

| Target | Source of truth | Update mechanism | Audience |
|--------|-----------------|------------------|----------|
| **Web (GitHub Pages)** | `docs/` directory on `main` | `./build_web.sh` then commit `docs/` | Players who use the online version |
| **Linux native** | `main.c` on `main` | `./build_native.sh` (developer's machine) | Developers / anyone with SDL3 installed |
| **Windows x86_64** | GitHub Actions release artefact | Tag `vX.Y.Z` triggers `.github/workflows/release.yml` | End users on Windows 10+ |

All three builds compile from the **same `main.c`** with platform branches under `#ifdef __EMSCRIPTEN__` / `#ifdef _WIN32`. Don't fork the source.

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
   Output: `docs/index.{html,js,wasm,data}`.
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

## Quick reference

```bash
# Update web (committed and pushed):
./build_web.sh && git add -A docs/ && git commit -m "..." && git push

# Cut a Windows release:
git tag v0.X.Y && git push origin v0.X.Y

# Watch the release build:
gh run watch $(gh run list --workflow=release.yml --limit 1 --json databaseId --jq '.[0].databaseId')
```
