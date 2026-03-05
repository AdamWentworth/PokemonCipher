#pragma once

#include "engine/Map.h"
#include "engine/TextureManager.h"
#include "engine/ecs/World.h"

namespace OverworldEntityFactory {
void createStaticCollisionEntities(World& world, const Map& map);
void createCameraEntity(World& world, const Map& map, int viewportWidth, int viewportHeight);
void createPlayerEntity(World& world, const Map& map, const Vector2D& spawnPoint, TextureManager& textureManager);
void resolvePlayerWallCollisions(World& world);
} // namespace OverworldEntityFactory
