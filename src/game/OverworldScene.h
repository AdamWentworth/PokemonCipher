#pragma once

#include <SDL3/SDL_events.h>

#include "engine/Map.h"
#include "engine/TextureManager.h"
#include "engine/TilemapRenderer.h"
#include "engine/ecs/World.h"
#include "engine/manager/Scene.h"
#include "game/systems/GridMovementSystem.h"

class OverworldScene : public Scene {
public:
    OverworldScene(
        TextureManager& textureManager,
        const char* mapPath,
        const char* baseTilesetPath,
        const char* coverTilesetPath,
        int viewportWidth,
        int viewportHeight
    );

    void handleEvent(const SDL_Event& event) override;
    void update(float dt) override;
    void render() override;

private:
    void createStaticCollisionEntities();
    void createCameraEntity(int viewportWidth, int viewportHeight);
    void createPlayerEntity();
    void resolveCollisions();

    TextureManager& textureManager_;
    Map map_;
    TilemapRenderer tilemapRenderer_;
    World world_;
    GridMovementSystem gridMovementSystem_;
};
