# PokemonCipher 🎮

A custom 2D engine plus game prototype for a Pokémon-style RPG built in C++20 with SDL3.

PokemonCipher is engine-first. The current game uses Pokémon FireRed as a baseline for assets, UI conventions, and core gameplay structure, but the runtime itself is a separate implementation focused on modernizing the exploration, scripting, UI, and battle pipeline on top of a custom engine.

> ⚠️ This repository is an educational/prototype project. It is not affiliated with Nintendo, Game Freak, or The Pokémon Company.

---

## 🛠️ Tech Stack
| Area | Tech |
| --- | --- |
| Language | C++20 |
| Build | CMake, vcpkg |
| Windowing / Input / Rendering | SDL3, SDL3_image |
| Data Formats | TMX |
| Scripting | Lua |
| Parsing / Utilities | tinyxml2|

---

## 📁 Repo Layout
- `src/` engine and game runtime code
- `assets/` canonical runtime assets
- `assets/config/` Lua-driven runtime registries and UI layout tuning
- `build/` generated build outputs

---

## 🧭 Gameplay Prototype Goals
Flow in the current prototype:
1. Boot into intro, overworld, or auto-start flow
2. Explore a map using tile-step movement and collision
3. Trigger scripts, warps, and encounter regions
4. Transition into a battle scene
5. Resolve battle flow and return to the overworld with state preserved

Current design direction:
- FireRed-style overworld
- Engine built independently from the original game
- Future mechanical extensions may include battle timing ideas such as dodging/blocking and Colosseum-inspired snagging concepts

The current priority is a reliable vertical slice, not a full campaign.

---

## ⌨️ Controls
- `WASD` or arrow keys to move

---

## 📜 Lua Scripting
Lua will be used for event/content scripting rather than hardcoding one-off gameplay flows in C++.

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
---