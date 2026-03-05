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

## Build

1. Configure: `cmake --preset vs2026`
2. Build: `cmake --build --preset debug`
3. Run: `build/Debug/PokemonCipher.exe`

## Regenerate Pallet Town Assets

Requires Python and Pillow:

`python tools/import_firered_pallet.py --firered-root C:\Code\pokefirered --project-root C:\Code\PokemonCipher`
