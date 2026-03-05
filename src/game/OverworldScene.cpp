#include "game/OverworldScene.h"

#include <iostream>

#include "engine/ecs/Component.h"
#include "engine/manager/AssetManager.h"

OverworldScene::OverworldScene(
    TextureManager& textureManager,
    const char* mapPath,
    const char* baseTilesetPath,
    const char* coverTilesetPath,
    const int viewportWidth,
    const int viewportHeight
) : textureManager_(textureManager),
    tilemapRenderer_(textureManager, baseTilesetPath, coverTilesetPath) {

    if (!map_.load(mapPath)) {
        std::cout << "Failed to load map: " << mapPath << '\n';
    }

    gridMovementSystem_.setWorldBounds(
        static_cast<float>(map_.width * map_.tileWidth),
        static_cast<float>(map_.height * map_.tileHeight)
    );

    createStaticCollisionEntities();
    createCameraEntity(viewportWidth, viewportHeight);
    createPlayerEntity();
}

void OverworldScene::handleEvent(const SDL_Event& event) {
    (void)event;
}

void OverworldScene::update(const float dt) {
    gridMovementSystem_.update(world_.entities(), dt);
    world_.update(dt);
    resolveCollisions();
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

void OverworldScene::createPlayerEntity() {
    if (!AssetManager::hasAnimation("player")) {
        AssetManager::loadAnimation("player", "assets/animations/red_overworld.xml");
    }

    Entity& player = world_.createEntity();

    const Vector2D spawn = map_.playerSpawn;
    auto& transform = player.addComponent<Transform>(spawn, 0.0f, 1.0f);
    player.addComponent<Velocity>(Vector2D(0.0f, 0.0f), 80.0f);
    player.addComponent<GridMovement>(static_cast<float>(map_.tileWidth), false, spawn);

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
