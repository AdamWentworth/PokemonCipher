#pragma once

#include "../ecs/World.h"
#include "../ecs/systems/DialogueSystem.h"
#include "../ecs/systems/StartMenuSystem.h"
#include "../ecs/systems/WaypointSystem.h"

namespace SceneSetup {
// Scene construction has grown into its own job, so this helper keeps the
// one-time map/entity setup out of Scene.cpp and leaves that file focused on
// what happens after the scene is already running.
void initialize(
    World& world,
    DialogueSystem& dialogueSystem,
    StartMenuSystem& startMenuSystem,
    WaypointSystem& waypointSystem,
    const char* mapPath,
    const char* tilesetPath,
    int windowWidth,
    int windowHeight,
    const SceneSpawnRequest& spawnRequest
);
}
