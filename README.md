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
- `startintro`, `resetintro`, `party` for intro/party testing
- `warp <mapId> [spawnId]` to jump maps
- `warptile <mapId> <tx> <ty>` and `warpxy <mapId> <x> <y>` for coordinate warps
- `goto <tx> <ty>` and `teleport <x> <y>` to reposition on current map
- `setflag/getflag` and `setvar/getvar` for script/game-state testing

## Lua Script Files

- `runscript <id>` loads `assets/scripts/<id>.lua`
- Scripts must `return` an array of command arrays
- Supported ops:
  `log`, `say`, `wait`, `lock_input`, `unlock_input`, `set_flag`, `set_var`,
  `clear_party`, `add_party`, `warp_spawn`, `warp_xy`, `teleport`
- Example: see `assets/scripts/dev_bootstrap_pallet.lua`

## Build

1. Configure: `cmake --preset vs2026`
2. Build: `cmake --build --preset debug`
3. Run: `build/Debug/PokemonCipher.exe`

Optional dev launch args:
- `--boot auto|intro|overworld`
- `--map <mapId>`
- `--spawn <spawnId>`

Optional env vars:
- `POKEMONCIPHER_BOOT_MODE`
- `POKEMONCIPHER_MAP`
- `POKEMONCIPHER_SPAWN`

## Asset Pipeline (Recommended)

Use the orchestrator to build normalized assets directly into canonical `assets/`:

`python tools/build_assets.py --firered-root C:\Code\pokefirered --project-root C:\Code\PokemonCipher`

Useful options:
- `--no-with-maps` to skip map import stage
- `--clean-assets` to wipe generated asset outputs before rebuilding
- `--assets-root <path>` to target a different asset output folder
- `--verify-manifest` to verify `assets/.build_manifest.json` against current import script hashes

Folders:
- `assets_raw/`: raw extraction workspace (not runtime)
- `assets/`: single source of truth for runtime assets

Generated map cache:
- Map imports now emit `*.tmx.pcmap` sidecar blobs.
- Runtime prefers `.pcmap` for fast map loads and falls back to `.tmx` automatically.

### Runtime Tunables

These are now data-driven and loaded at runtime from Lua config files:
- `assets/config/maps/map_registry.lua`: map IDs, TMX paths, tileset paths, aliases
- `assets/config/pokemon/species_registry.lua`: species names/types/abilities/move lists/icon+sprite paths
- `assets/config/world/character_assets.lua`: player + NPC default sprite/animation paths and player draw scale
- `assets/config/ui/party_menu_layout.lua`: party menu layout anchors/offsets
- `assets/config/ui/pokemon_summary_layout.lua`: summary UI anchors/offsets/gauge positions

Source-controlled templates live in `assets/config/*`.

## Individual Import Scripts

All import scripts support `--assets-root` if you want to run a single stage.

Examples:
- `python tools/import_firered_pallet.py --firered-root C:\Code\pokefirered --project-root C:\Code\PokemonCipher --assets-root C:\Code\PokemonCipher\assets --map-name PalletTown`
- `python tools/import_firered_party_menu.py --firered-root C:\Code\pokefirered --project-root C:\Code\PokemonCipher --assets-root C:\Code\PokemonCipher\assets`
- `python tools/import_firered_summary_assets.py --firered-root C:\Code\pokefirered --project-root C:\Code\PokemonCipher --assets-root C:\Code\PokemonCipher\assets`
