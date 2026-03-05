#include "game/OverworldScene.h"

#include <algorithm>
#include <cctype>
#include <iostream>

#include "engine/ecs/Component.h"
#include "engine/manager/AssetManager.h"
#include "engine/utils/Collision.h"

OverworldScene::OverworldScene(
    TextureManager& textureManager,
    const MapRegistry& mapRegistry,
    const std::string& initialMapId,
    const int viewportWidth,
    const int viewportHeight
) : textureManager_(textureManager),
    mapRegistry_(mapRegistry),
    viewportWidth_(viewportWidth),
    viewportHeight_(viewportHeight),
    tilemapRenderer_(textureManager, nullptr, nullptr) {
    if (!loadMap(initialMapId, "default")) {
        std::cout << "Failed to load initial map id: " << initialMapId << '\n';
    }
}

void OverworldScene::handleEvent(const SDL_Event& event) {
    (void)event;
}

void OverworldScene::update(const float dt) {
    gridMovementSystem_.update(world_.entities(), dt);
    world_.update(dt);
    resolveCollisions();
    checkMapWarps(dt);
    world_.updateCamera();
}

void OverworldScene::render() {
    const Entity* cameraEntity = world_.findFirstWith<Camera>();
    if (cameraEntity) {
        const auto& camera = cameraEntity->getComponent<Camera>();
        tilemapRenderer_.renderGround(map_, camera);
    }

    world_.render(textureManager_);

    if (cameraEntity) {
        const auto& camera = cameraEntity->getComponent<Camera>();
        tilemapRenderer_.renderCover(map_, camera);
    }
}

bool OverworldScene::warpTo(const std::string& mapId, const std::string& spawnId) {
    return loadMap(mapId, spawnId);
}

bool OverworldScene::loadMap(
    const std::string& mapId,
    const std::string& spawnId,
    const std::optional<Vector2D>& spawnOverride
) {
    const std::string normalizedMapId = normalizeMapKey(mapId);
    const MapDefinition* mapDefinition = mapRegistry_.find(normalizedMapId);
    if (!mapDefinition) {
        std::cout << "Map id '" << mapId << "' is not registered.\n";
        return false;
    }

    if (!map_.load(mapDefinition->mapPath.c_str())) {
        std::cout << "Failed to load map path: " << mapDefinition->mapPath << '\n';
        return false;
    }

    currentMapId_ = mapDefinition->id;
    tilemapRenderer_.setTilesets(
        mapDefinition->baseTilesetPath.c_str(),
        mapDefinition->coverTilesetPath.empty() ? nullptr : mapDefinition->coverTilesetPath.c_str()
    );

    world_.clear();

    gridMovementSystem_.setWorldBounds(
        static_cast<float>(map_.width * map_.tileWidth),
        static_cast<float>(map_.height * map_.tileHeight)
    );

    createStaticCollisionEntities();
    createCameraEntity(viewportWidth_, viewportHeight_);

    const Vector2D spawnPoint = spawnOverride.has_value() ? *spawnOverride : map_.getSpawnPoint(spawnId);
    createPlayerEntity(spawnPoint);

    warpCooldownSeconds_ = 0.2f;
    return true;
}

void OverworldScene::checkMapWarps(const float dt) {
    if (warpCooldownSeconds_ > 0.0f) {
        warpCooldownSeconds_ = std::max(0.0f, warpCooldownSeconds_ - dt);
        return;
    }

    Entity* player = world_.findFirstWith<PlayerTag>();
    if (!player || !player->hasComponent<Collider>()) {
        return;
    }

    const auto& playerCollider = player->getComponent<Collider>().rect;
    for (const WarpPoint& warp : map_.warpPoints) {
        if (!Collision::AABB(playerCollider, warp.area)) {
            continue;
        }

        std::string targetMap = normalizeMapKey(warp.targetMap);
        if (targetMap.rfind("map_", 0) == 0) {
            targetMap = targetMap.substr(4);
        }

        std::optional<Vector2D> targetSpawnOverride;
        if (warp.targetSpawn.x != 0.0f || warp.targetSpawn.y != 0.0f) {
            targetSpawnOverride = warp.targetSpawn;
        }

        if (!loadMap(targetMap, "default", targetSpawnOverride)) {
            std::cout << "Warp target map not available: " << warp.targetMap << '\n';
        }

        return;
    }
}

std::string OverworldScene::normalizeMapKey(std::string key) {
    std::transform(key.begin(), key.end(), key.begin(), [](const unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return key;
}

void OverworldScene::createStaticCollisionEntities() {
    for (const SDL_FRect& rect : map_.blockingRects) {
        Entity& wall = world_.createEntity();

        wall.addComponent<Transform>(Vector2D(rect.x, rect.y), 0.0f, 1.0f);

        auto& collider = wall.addComponent<Collider>();
        collider.tag = "wall";
        collider.rect = rect;
    }
}

void OverworldScene::createCameraEntity(const int viewportWidth, const int viewportHeight) {
    Entity& cameraEntity = world_.createEntity();

    SDL_FRect cameraView{};
    cameraView.w = static_cast<float>(viewportWidth);
    cameraView.h = static_cast<float>(viewportHeight);

    cameraEntity.addComponent<Camera>(
        cameraView,
        static_cast<float>(map_.width * map_.tileWidth),
        static_cast<float>(map_.height * map_.tileHeight)
    );
}

void OverworldScene::createPlayerEntity(const Vector2D& spawnPoint) {
    if (!AssetManager::hasAnimation("player")) {
        AssetManager::loadAnimation("player", "assets/animations/red_overworld.xml");
    }

    Entity& player = world_.createEntity();

    auto& transform = player.addComponent<Transform>(spawnPoint, 0.0f, 1.0f);
    player.addComponent<Velocity>(Vector2D(0.0f, 0.0f), 80.0f);
    player.addComponent<GridMovement>(static_cast<float>(map_.tileWidth), false, spawnPoint);

    const Animation animation = AssetManager::getAnimation("player");
    auto& anim = player.addComponent<Animation>(animation);
    anim.currentClip = "idle_down";
    anim.baseSpeed = 8.0f / 60.0f;
    anim.speed = anim.baseSpeed;

    SDL_FRect src{0.0f, 0.0f, 16.0f, 32.0f};
    const auto clipIt = anim.clips.find(anim.currentClip);
    if (clipIt != anim.clips.end() && !clipIt->second.frameIndices.empty()) {
        src = clipIt->second.frameIndices[0];
    }

    SDL_FRect dst{transform.position.x, transform.position.y, 16.0f, 32.0f};
    auto& sprite = player.addComponent<Sprite>(textureManager_.load("assets/characters/red_normal.png"), src, dst);
    sprite.offset = Vector2D(0.0f, -16.0f);
    sprite.mirrorOnRightClip = true;

    auto& collider = player.addComponent<Collider>();
    collider.tag = "player";
    collider.rect.w = static_cast<float>(map_.tileWidth);
    collider.rect.h = static_cast<float>(map_.tileHeight);
    collider.rect.x = transform.position.x;
    collider.rect.y = transform.position.y;

    player.addComponent<PlayerTag>();
}

void OverworldScene::resolveCollisions() {
    for (const CollisionPair& pair : world_.collisions()) {
        if (!pair.entityA || !pair.entityB) {
            continue;
        }

        if (!pair.entityA->hasComponent<Collider>() || !pair.entityB->hasComponent<Collider>()) {
            continue;
        }

        auto& colliderA = pair.entityA->getComponent<Collider>();
        auto& colliderB = pair.entityB->getComponent<Collider>();

        Entity* player = nullptr;
        if (colliderA.tag == "player" && colliderB.tag == "wall") {
            player = pair.entityA;
        } else if (colliderA.tag == "wall" && colliderB.tag == "player") {
            player = pair.entityB;
        }

        if (!player || !player->hasComponent<Transform>()) {
            continue;
        }

        auto& transform = player->getComponent<Transform>();
        transform.position = transform.oldPosition;
    }
}
