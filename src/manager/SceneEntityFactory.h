#pragma once

#include "../ecs/World.h"
#include "../ecs/systems/WaypointSystem.h"

namespace SceneEntityFactory {
// Scene entity creation has its own reason to change, so this factory keeps
// player, camera, walls, and map visuals out of SceneSetup's map-loading job.
void populate(
    World& world,
    WaypointSystem& waypointSystem,
    int windowWidth,
    int windowHeight,
    const SceneSpawnRequest& spawnRequest
);
}
