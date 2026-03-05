# PokemonCipher

Pokemon-style overworld prototype in C++20 using SDL3.

## Current Scope

- Grid/tile-step player movement
- FireRed Pallet Town import pipeline (`tools/import_firered_pallet.py`)
- Collision + blocked-turn bump animation behavior
- Water blocked for on-foot movement
- Foreground cover layer rendering (player can move behind buildings/roofs)
- Logical rendering with integer scaling for crisp pixel output

## Controls

- `WASD` or arrow keys to move
- <code>`</code> to toggle developer console

## Developer Console (for testing)

- `help` to list commands
- `maps` / `map` / `spawns` / `pos` for quick map inspection
- `scripts`, `runscript <id>`, `stopscript` for event-sequence testing
- `warp <mapId> [spawnId]` to jump maps
- `warptile <mapId> <tx> <ty>` and `warpxy <mapId> <x> <y>` for coordinate warps
- `goto <tx> <ty>` and `teleport <x> <y>` to reposition on current map
- `setflag/getflag` and `setvar/getvar` for script/game-state testing

## Lua Script Files

- `runscript <id>` loads `assets/scripts/<id>.lua`
- Scripts must `return` an array of command arrays
- Supported ops:
  `log`, `wait`, `lock_input`, `unlock_input`, `set_flag`, `set_var`, `warp_spawn`, `warp_xy`, `teleport`
- Example: see `assets/scripts/dev_bootstrap_pallet.lua`

## Build

1. Configure: `cmake --preset vs2026`
2. Build: `cmake --build --preset debug`
3. Run: `build/Debug/PokemonCipher.exe`

## Regenerate Pallet Town Assets

Requires Python and Pillow:

`python tools/import_firered_pallet.py --firered-root C:\Code\pokefirered --project-root C:\Code\PokemonCipher`
