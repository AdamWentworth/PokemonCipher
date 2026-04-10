# PokemonCipher

PokemonCipher is a C++20 / SDL3 Pokemon-style overworld prototype.

The current repo is focused on exploration systems and map flow rather than a full RPG. It has connected maps, tile movement, collisions, warps, a simple start menu shell, one working interaction example, and random grass encounters presented as dialogue popups.

This is an educational prototype project and is not affiliated with Nintendo, Game Freak, or The Pokemon Company.

## Current Scope

What is currently implemented:

- Connected overworld maps for Pallet Town, Route 1, Player's House 1F, Player's House 2F, Rival's House, and Oak's Lab
- Tile-based movement and camera
- TMX map loading
- Collision, cover tiles, spawn points, and warp points
- Random wild encounter popups in Route 1 grass
- One working interaction in Oak's Lab
- A simple start menu with placeholder entries

What is not implemented yet:

- Battle scenes
- Save/load
- Full NPC scripting/content systems
- Finished menu screens

## Controls

- Move: `WASD` or arrow keys
- Confirm / interact / advance dialogue: `Enter`, `Numpad Enter`, `Space`, or `Z`
- Open the start menu while standing still: `Esc`, `Tab`, or `Enter`
- Move the menu cursor: `W` / `S` or Up / Down
- Close the menu: `Esc`, `X`, or `Backspace`

## Run Without Compiling

If you just want to run the project on Windows, use the tracked runtime package:

```powershell
release\windows\PokemonCipher.exe
```

That folder includes:

- `PokemonCipher.exe`
- required DLLs
- a local copy of the trimmed runtime `assets/` folder

The duplicated `assets/` folder is intentional here so the packaged executable works directly after clone without depending on the generated CMake build tree.

## Build From Source (Windows)

Requirements:

- Visual Studio 2026
- CMake 3.22+
- vcpkg

Important: the current preset in `CMakePresets.json` assumes vcpkg is installed at `C:\dev\vcpkg`. If your vcpkg installation lives somewhere else, update that file before configuring.

Configure and build:

```powershell
cmake --preset vs2026
cmake --build --preset release
```

Run the locally built executable:

```powershell
build\Release\PokemonCipher.exe
```

The build copies `assets/` next to the executable automatically.

## Repo Layout

- `src/` - C++ source code
- `assets/` - trimmed runtime assets used by the current prototype build
- `build/` - local CMake output (ignored by git)
- `release/windows/` - tracked Windows runtime folder for running the game without compiling

## Tech Stack

| Area | Tech |
| --- | --- |
| Language | C++20 |
| Build | CMake, vcpkg |
| Windowing / Input / Rendering | SDL3, SDL3_image |
| Data Formats | TMX, XML |
| Parsing / Utilities | tinyxml2 |
| Extra Dependency | Lua is still listed in `vcpkg.json`, but it is not wired into the current runtime |

## Architecture

Start with this view if you are new to the repo. It shows the main runtime layers and the handoff from the game loop into scene management, world state, and the engine/gameplay systems that operate on that state.

```mermaid
---
config:
  theme: neo-dark
---
classDiagram
    direction LR

    class Game {
        +init()
        +handleEvents()
        +update()
        +render()
        +destroy()
        Game.cpp
        GameBootstrap.cpp
        GameContent.cpp
    }

    class SceneManager {
        SceneManager.h
    }

    class Scene {
        Scene.cpp
        SceneSetup.cpp
        SceneEntityFactory.cpp
        SceneRuntimeHelpers.cpp
    }

    class World {
        World.h
        World.cpp
    }

    class Map {
        Map.h
        Map.cpp
        MapLoader.h
        MapLoader.cpp
    }

    class TextureManager {
        TextureManager.h
        TextureManager.cpp
    }

    class AssetManager {
        AssetManager.h
        AssetManager.cpp
    }

    class Entity {
        Entity.h
    }

    class Component {
        Component.h
        Transform
        GridMovement
        Sprite
        Collider
        Animation
        Camera
        PlayerTag
    }

    class EngineSystems {
        KeyboardInputSystem
        MovementSystem
        CollisionSystem
        AnimationSystem
        CameraSystem
        RenderSystem
    }

    class GameSystems {
        InteractionSystem
        EncounterSystem
        DialogueSystem
        StartMenuSystem
        WaypointSystem
    }

    Game --> SceneManager : drives
    Game --> TextureManager : uses
    Game --> AssetManager : loads startup assets

    SceneManager --> Scene : owns current scene
    Scene --> World : coordinates
    Scene --> GameSystems : coordinates

    World *-- Map : owns
    World o-- Entity : owns many
    World --> EngineSystems : updates

    Map --> TextureManager : draws with

Entity o-- Component : carries
```

## Detailed Architecture Views

These follow-up diagrams zoom in on one slice of the runtime at a time. They are best read after the high-level view above, and they are meant to clarify responsibilities rather than act as a complete file-by-file reference.

### Game Runtime

This view covers startup and the main loop. It shows how the executable bootstraps SDL, loads startup content, and hands frame-by-frame work to the scene layer.

Files: `main.cpp`, `Game.cpp`, `GameBootstrap.cpp`, `GameContent.cpp`

- start the program
- initialize SDL and the game view
- create the window and renderer
- load startup assets
- register the available scenes
- run the main loop through `SceneManager`

```mermaid
---
config:
  theme: neo-dark
---
classDiagram
    direction TB

    class Main {
        main.cpp
    }

    class Game {
        Game.h
        Game.cpp
    }

    class GameBootstrap {
        GameBootstrap.cpp
        SDL setup
        window
        renderer
        game view
    }

    class GameContent {
        GameContent.cpp
        scene registration
        startup animation load
    }

    class SceneManager {
        SceneManager.h
    }

    class AssetManager {
        AssetManager.h
        AssetManager.cpp
    }

    Main --> Game : creates and runs
    Game --> GameBootstrap : initializes runtime
    Game --> GameContent : loads startup content
    GameContent --> AssetManager : loads startup animation
    GameContent --> SceneManager : registers scenes
    Game --> SceneManager : updates and renders
```

### Scene Orchestration

This is the map and scene coordination layer. It shows how scene definitions become active runtime scenes, how maps are loaded, and where control passes into the world update/render flow.

Files: `SceneManager.h`, `Scene.h`, `Scene.cpp`

- register scene parameters
- create and switch scenes
- load the active map
- build scene startup state
- coordinate scene-level flow
- hand update and render to `World`

```mermaid
---
config:
  theme: neo-dark
---
classDiagram
    direction TB

    class SceneManager {
        SceneManager.h
    }

    class Scene {
        Scene.h
        Scene.cpp
        map load
        startup setup
        scene flow
    }

    class World {
        World.h
        World.cpp
    }

    class Map {
        Map.h
        Map.cpp
        MapLoader.cpp
    }

    SceneManager --> Scene : creates and switches
    Scene --> World : updates and renders
    Scene --> Map : loads active map
    World --> Map : owns active map
```

### ECS Core

This is the runtime data layer. It shows where entities and components live, how systems access shared state, and where lightweight event flow fits into the world model.

Files: `World.h`, `World.cpp`, `Entity.h`, `Component.h`, `EventManager.h`

- own the active world state
- store entities in `World`
- attach data through components
- expose shared state to systems
- support deferred entity creation
- publish simple collision events

```mermaid
---
config:
  theme: neo-dark
---
classDiagram
    direction TB

    class World {
        World.h
        World.cpp
        entities
        deferred entities
        event manager
    }

    class Entity {
        Entity.h
        active flag
        component bitset
        component array
    }

    class Component {
        Component.h
        Transform
        GridMovement
        Sprite
        Collider
        Animation
        Camera
        PlayerTag
    }

    class EventManager {
        EventManager.h
        CollisionEvent
        emit()
        subscribe()
    }

    class Systems {
        src/ecs/systems/*
    }

    World o-- Entity : owns many
    Entity o-- Component : carries data
    World --> Systems : exposes shared state
    World --> EventManager : owns events
    Systems --> EventManager : emit and listen
```

### Gameplay Systems

This slice focuses on higher-level overworld behavior rather than low-level simulation. These systems sit close to scene flow and player-facing interactions such as talking, encounters, menus, and warps.

Files: `InteractionSystem.h`, `EncounterSystem.h`, `DialogueSystem.h`, `StartMenuSystem.h`, `WaypointSystem.h`

- check the tile in front of the player
- trigger scene-level interactions
- trigger wild encounters after movement
- open and close dialogue flow
- open and handle the start menu
- switch maps through waypoint rules

```mermaid
---
config:
  theme: neo-dark
---
classDiagram
    direction TB

    class Scene {
        Scene.cpp
    }

    class World {
        World.h
        World.cpp
    }

    class Map {
        Map.h
        Map.cpp
    }

    class InteractionSystem {
        InteractionSystem.h
    }

    class EncounterSystem {
        EncounterSystem.h
    }

    class DialogueSystem {
        DialogueSystem.h
    }

    class StartMenuSystem {
        StartMenuSystem.h
    }

    class WaypointSystem {
        WaypointSystem.h
    }

    Scene --> InteractionSystem : checks front tile
    Scene --> EncounterSystem : checks landed tile
    Scene --> DialogueSystem : opens and closes
    Scene --> StartMenuSystem : opens and handles
    Scene --> WaypointSystem : checks warps

    Scene --> World : reads player state
    World --> Map : owns map data
    InteractionSystem --> Map : reads interactions
    EncounterSystem --> Map : reads encounter zones
    WaypointSystem --> Map : reads warp points
```

### Engine Systems

This is the lower-level frame simulation and rendering pass. It covers the systems that update movement, collision, animation, camera state, and drawing against the active map each frame.

Files: `KeyboardInputSystem.h`, `MovementSystem.h`, `CollisionSystem.*`, `AnimationSystem.h`, `CameraSystem.h`, `RenderSystem.h`

- read raw keyboard input
- move entities through the world
- block invalid movement and collisions
- update animation and camera state
- render terrain, entities, and cover
- run the lower-level frame simulation

```mermaid
---
config:
  theme: neo-dark
---
classDiagram
    direction TB

    class World {
        World.h
        World.cpp
    }

    class KeyboardInputSystem {
        KeyboardInputSystem.h
    }

    class MovementSystem {
        MovementSystem.h
    }

    class CollisionSystem {
        CollisionSystem.h
        CollisionSystem.cpp
    }

    class AnimationSystem {
        AnimationSystem.h
    }

    class CameraSystem {
        CameraSystem.h
    }

    class RenderSystem {
        RenderSystem.h
    }

    class Map {
        Map.h
        Map.cpp
    }

    class TextureManager {
        TextureManager.h
        TextureManager.cpp
    }

    World --> KeyboardInputSystem : runs
    World --> MovementSystem : runs
    World --> CollisionSystem : runs
    World --> AnimationSystem : runs
    World --> CameraSystem : runs
    World --> RenderSystem : runs

    MovementSystem --> Map : checks movement space
    CollisionSystem --> Map : checks colliders
    RenderSystem --> Map : draws terrain and cover
    RenderSystem --> TextureManager : draws textures
```

### Assets and World Data

This view explains how external files become usable runtime state. It covers animation loading, texture caching, TMX parsing, and how that loaded data feeds scene startup and rendering.

Files: `AssetManager.*`, `TextureManager.*`, `Map.h`, `Map.cpp`, `MapLoader.h`, `MapLoader.cpp`

- load animation data from XML
- cache textures for reuse
- parse TMX map files
- store tile and object-layer data
- provide resources to game and scene startup
- provide map data to rendering

```mermaid
---
config:
  theme: neo-dark
---
classDiagram
    direction TB

    class Game {
        Game.cpp
        GameContent.cpp
    }

    class Scene {
        Scene.cpp
    }

    class AssetManager {
        AssetManager.h
        AssetManager.cpp
        animation cache
        XML load
    }

    class TextureManager {
        TextureManager.h
        TextureManager.cpp
        texture cache
        texture draw
    }

    class Map {
        Map.h
        Map.cpp
        tile data
        object data
    }

    class MapLoader {
        MapLoader.h
        MapLoader.cpp
        TMX parse
    }

    Game --> AssetManager : loads startup animations
    Scene --> AssetManager : reads animation data
    Scene --> TextureManager : loads textures
    Scene --> Map : loads active map
    Map --> MapLoader : fills map data
    Map --> TextureManager : draws tiles
```
