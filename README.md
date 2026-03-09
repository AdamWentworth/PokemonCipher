# PokemonCipher 🎮

A custom 2D engine plus game prototype for a Pokémon-style RPG built in C++20 with SDL3.

PokemonCipher is engine-first. The current game uses Pokémon FireRed as a baseline for assets, UI conventions, and core gameplay structure, but the runtime itself is a separate implementation focused on modernizing the exploration, scripting, UI, and battle pipeline on top of a custom engine.

> ⚠️ This repository is an educational/prototype project. It is not affiliated with Nintendo, Game Freak, or The Pokémon Company.

---

## 🚧 Current Status
- Engine core: application loop, SDL3 windowing, input handling, renderer setup, scene management
- World runtime: tilemap loading, compiled `.pcmap` sidecar cache, collision, spawn points, warps, encounter regions
- Rendering: camera-relative tile rendering, foreground cover layers, logical rendering with integer scaling for crisp pixels
- Overworld gameplay: grid/tile-step movement, blocked-turn bump feedback, NPC interaction hooks, tall-grass encounter checks
- Scripting: Lua-driven event sequences for dialogue, waits, flags, variables, party edits, and warps
- Battle prototype: encounter handoff, command menu, message queue, damage resolution, battle exit callback
- UI: dialogue box, start menu, party menu, and Pokémon summary screens with runtime layout config
- Persistence and debug: save/load support plus developer console commands for maps, scripts, flags, vars, warps, and party setup
- Asset pipeline: FireRed-derived map and UI import scripts, normalized runtime `assets/`, and configurable Lua registries

---

## 🛠️ Tech Stack
| Area | Tech |
| --- | --- |
| Language | C++20 |
| Build | CMake, vcpkg |
| Windowing / Input / Rendering | SDL3, SDL3_image |
| Data Formats | TMX, compiled `.pcmap` sidecar cache |
| Scripting | Lua |
| Parsing / Utilities | tinyxml2, Python asset-import tools |

---

## 📁 Repo Layout
- `src/` engine and game runtime code
- `assets/` canonical runtime assets
- `assets_raw/` raw extraction / import workspace
- `tools/` import scripts and asset build orchestration
- `assets/config/` Lua-driven runtime registries and UI layout tuning
- `build/` generated build outputs

---

## 🧭 Gameplay Prototype
Flow in the current prototype:
1. Boot into intro, overworld, or auto-start flow
2. Explore a map using tile-step movement and collision
3. Trigger scripts, warps, and encounter regions
4. Transition into a battle scene
5. Resolve battle flow and return to the overworld with state preserved

Current design direction:
- FireRed-style overworld and UI baseline
- Engine and tooling built independently from the original game runtime
- Future mechanical extensions may include battle timing ideas such as dodging/blocking and Colosseum-inspired snagging concepts

The current priority is a reliable vertical slice, not a full campaign.

---

## ⌨️ Controls
- `WASD` or arrow keys to move
- <code>`</code> to toggle the developer console

---

## 🧪 Developer Console
Use the console for fast testing and iteration.

Inspection and map commands:
- `help`
- `maps`
- `map`
- `spawns`
- `pos`

Script and event commands:
- `scripts`
- `runscript <id>`
- `stopscript`

Intro / progression / party commands:
- `startintro`
- `resetintro`
- `party`

Warp and movement commands:
- `warp <mapId> [spawnId]`
- `warptile <mapId> <tx> <ty>`
- `warpxy <mapId> <x> <y>`
- `goto <tx> <ty>`
- `teleport <x> <y>`

State editing commands:
- `setflag <name> <0|1>`
- `getflag <name>`
- `setvar <name> <value>`
- `getvar <name>`

These commands exist to reduce iteration cost while building and testing incomplete content.

---

## 📜 Lua Scripting
Lua is used for event/content scripting rather than hardcoding one-off gameplay flows in C++.

### Script loading
- `runscript <id>` loads `assets/scripts/<id>.lua`
- Scripts must `return` an array of command arrays

### Supported operations
- `log`
- `say`
- `wait`
- `lock_input`
- `unlock_input`
- `set_flag`
- `set_var`
- `clear_party`
- `add_party`
- `warp_spawn`
- `warp_xy`
- `teleport`

Example:
- `assets/scripts/dev_bootstrap_pallet.lua`

---

## ⚙️ Runtime Data and Config
Several runtime systems are data-driven and loaded from Lua config files:

- `assets/config/maps/map_registry.lua`  
  Map IDs, TMX paths, tileset paths, aliases, and registry data

- `assets/config/pokemon/species_registry.lua`  
  Species names, types, abilities, move lists, icon paths, and sprite paths

- `assets/config/world/character_assets.lua`  
  Player and NPC sprite/animation paths plus player draw scale

- `assets/config/ui/party_menu_layout.lua`  
  Party menu anchors, offsets, and layout tuning

- `assets/config/ui/pokemon_summary_layout.lua`  
  Summary screen anchors, offsets, and gauge positions

Source-controlled templates live under `assets/config/`.

---

## 🧱 Asset Pipeline
Runtime assets are built into canonical `assets/` outputs.

Recommended full build:

```powershell
python tools/build_assets.py --firered-root C:\Code\pokefirered --project-root C:\Code\PokemonCipher
```

Useful options:
- `--no-with-maps` to skip map import
- `--clean-assets` to wipe generated outputs before rebuilding
- `--assets-root <path>` to target a different asset root
- `--verify-manifest` to validate `assets/.build_manifest.json` against current script hashes

Folders:
- `assets_raw/` raw extraction workspace, not used directly at runtime
- `assets/` single source of truth for runtime assets

### Generated map cache
Map imports emit `*.tmx.pcmap` sidecar blobs.
Runtime prefers `.pcmap` for faster loads and falls back to `.tmx` automatically.

---

## 🔧 Individual Import Scripts
All import scripts support `--assets-root` if you want to run a single stage manually.

Examples:

```powershell
python tools/import_firered_pallet.py --firered-root C:\Code\pokefirered --project-root C:\Code\PokemonCipher --assets-root C:\Code\PokemonCipher\assets --map-name PalletTown
python tools/import_firered_party_menu.py --firered-root C:\Code\pokefirered --project-root C:\Code\PokemonCipher --assets-root C:\Code\PokemonCipher\assets
python tools/import_firered_summary_assets.py --firered-root C:\Code\pokefirered --project-root C:\Code\PokemonCipher --assets-root C:\Code\PokemonCipher\assets
```

---

## ▶️ Getting Started (Windows)
Requirements:
- Visual Studio 2026 or newer
- CMake
- vcpkg installed and `VCPKG_ROOT` set

Build with presets:

```powershell
cmake --preset vs2026
cmake --build --preset debug
```

Run:

```powershell
build\Debug\PokemonCipher.exe
```

Optional dev launch arguments:
- `--boot auto|intro|overworld`
- `--map <mapId>`
- `--spawn <spawnId>`

Optional environment variables:
- `POKEMONCIPHER_BOOT_MODE`
- `POKEMONCIPHER_MAP`
- `POKEMONCIPHER_SPAWN`

---

## 🗺️ Roadmap
Near-term:
- Stabilize overworld to battle to overworld state handoff
- Expand script coverage for authored events and progression flow
- Improve battle prototype reliability and UI feedback
- Continue moving tunables and content references into data files
- Add more validation and debug tooling around imported assets and registries

Longer-term:
- Broaden map coverage beyond the current prototype slice
- Extend battle mechanics beyond the current baseline flow
- Explore gameplay deviations such as dodging/blocking and snagging systems
- Improve presentation polish, audio integration, and content scale

---

## 📝 Notes
- Windows-first development right now
- The project reuses FireRed-derived assets and conventions as a prototype baseline, but the engine/runtime is a separate implementation
- The main technical focus is engine structure, tooling, scripting, and modernization of a classic Pokémon-style RPG loop
