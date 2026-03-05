#pragma once

#include <SDL3/SDL_events.h>

#include <optional>
#include <string>

#include "engine/Map.h"
#include "engine/TextureManager.h"
#include "engine/TilemapRenderer.h"
#include "engine/ecs/World.h"
#include "engine/manager/Scene.h"
#include "game/systems/GridMovementSystem.h"
#include "game/world/MapRegistry.h"

class OverworldScene : public Scene {
public:
    OverworldScene(
        TextureManager& textureManager,
        const MapRegistry& mapRegistry,
        const std::string& initialMapId,
        int viewportWidth,
        int viewportHeight
    );

    void handleEvent(const SDL_Event& event) override;
    void update(float dt) override;
    void render() override;

    bool warpTo(const std::string& mapId, const std::string& spawnId = "default");
    const std::string& currentMapId() const { return currentMapId_; }

private:
    bool loadMap(
        const std::string& mapId,
        const std::string& spawnId,
        const std::optional<Vector2D>& spawnOverride = std::nullopt
    );
    void checkMapWarps(float dt);
    static std::string normalizeMapKey(std::string key);
    void createStaticCollisionEntities();
    void createCameraEntity(int viewportWidth, int viewportHeight);
    void createPlayerEntity(const Vector2D& spawnPoint);
    void resolveCollisions();

    TextureManager& textureManager_;
    const MapRegistry& mapRegistry_;
    std::string currentMapId_;
    int viewportWidth_ = 0;
    int viewportHeight_ = 0;
    float warpCooldownSeconds_ = 0.0f;

    Map map_;
    TilemapRenderer tilemapRenderer_;
    World world_;
    GridMovementSystem gridMovementSystem_;
};
