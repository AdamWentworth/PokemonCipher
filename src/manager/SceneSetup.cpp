#include "SceneSetup.h"

#include "../TextureManager.h"
#include "SceneEntityFactory.h"

namespace SceneSetup {
void initialize(
    World& world,
    DialogueSystem& dialogueSystem,
    StartMenuSystem& startMenuSystem,
    WaypointSystem& waypointSystem,
    const char* mapPath,
    const char* tilesetPath,
    const int windowWidth,
    const int windowHeight,
    const SceneSpawnRequest& spawnRequest
) {
    dialogueSystem.setViewportSize(windowWidth, windowHeight);
    startMenuSystem.setViewportSize(windowWidth, windowHeight);

    // Loading the map and building its one-time entities is now kept here so
    // Scene.cpp can stay focused on per-frame gameplay flow.
    world.getMap().load(mapPath, TextureManager::load(tilesetPath));
    // Entity creation moved into its own factory so SceneSetup stays focused
    // on bootstrapping the scene instead of knowing each entity's details.
    SceneEntityFactory::populate(world, waypointSystem, windowWidth, windowHeight, spawnRequest);
}
}
