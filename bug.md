# [FIXED] Bug: Game Over Geometry Always Reverts to Geometry 1

**Note: This bug has been fixed.**

## Problem Description

When the player loses all lives and triggers a game over state:

1. **Expected behavior**: A random tunnel geometry (0-3) is selected, the game freezes with "PRESS ANY KEY" displayed, and when the player presses any key, the game should restart with that randomly selected geometry.

2. **Actual behavior**: A random geometry is initially selected and displayed, but when the player presses any key to continue, the game always restarts with geometry 1 (TUNNEL_CIRCLE), regardless of which geometry was originally selected.

## Current Implementation

### Game Over Trigger (`TriggerGameOverShape()`)
- Called when `lives <= 0`
- Generates a random shape: `randomShape = (TunnelShape)(rand() % 4)` (values 0-3)
- Sets both `ctx->gameOverShape` and `ctx->selectedTunnelShape` to this random value
- Calls `ApplyTunnelShape(ctx, randomShape)` to apply it immediately
- Game freezes at this point (main loop skips game logic when `gameOver == true`)

### Key Press Handling
When the player presses a key during game over:
- First, `RestartWithShape()` checks if they pressed 0-3 to manually select a geometry
- If not a 0-3 key, `ContinueGameWithSelectedGeometry()` is called
- `ContinueGameWithSelectedGeometry()` resets game state (lives, score, shots, enemies) and calls `ApplyTunnelShape(ctx, ctx->gameOverShape)`
- Game resumes with `gameOver = false`

### Problem Manifestation
- Debug output would show what geometry was selected and what geometry is being restored
- But visually, the tunnel always appears as geometry 1 after restart

## Possible Root Causes Under Investigation

1. **`SelectTunnelShape()` validation**: Has a check that defaults to TUNNEL_CIRCLE if shape is out of range, but shouldn't trigger for valid 0-3 values
2. **Unintended key matching**: Perhaps a key press is matching both the "any key" handler AND a 0-3 geometry selector somehow
3. **State synchronization issue**: `tunnelShape` (used for rendering) may not be syncing properly with `selectedTunnelShape` or `gameOverShape`
4. **Initialization issue**: `selectedTunnelShape` defaults to TUNNEL_CIRCLE and may not be properly overwritten before game over is triggered
5. **Timing issue**: Something might be resetting the geometry on the frame after restart before it's rendered

## Code Locations

- `TriggerGameOverShape()`: line ~245
- `ContinueGameWithSelectedGeometry()`: line ~470  
- Event handling for game over keys: line ~550
- `SelectTunnelShape()`: line ~238
- `ApplyTunnelShape()`: line ~231
- Tunnel drawing code: line ~663 (uses `ctx->tunnelShape` for all rendering)

## Next Steps

- Run with stderr capture to see the debug output messages
- Verify what geometry values are logged during selection and restoration
- Check if the issue is deterministic (always reverts to 1) or varies
- Trace through state changes frame-by-frame during restart
