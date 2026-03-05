#include "game/world/OverworldEntityFactory.h"

#include <algorithm>
#include <cmath>

#include "engine/ecs/Component.h"
#include "engine/manager/AssetManager.h"

namespace OverworldEntityFactory {
void createStaticCollisionEntities(World& world, const Map& map) {
    for (const SDL_FRect& rect : map.blockingRects) {
        Entity& wall = world.createEntity();
        wall.addComponent<Transform>(Vector2D(rect.x, rect.y), 0.0f, 1.0f);

        auto& collider = wall.addComponent<Collider>();
        collider.tag = "wall";
        collider.rect = rect;
    }
}

void createCameraEntity(World& world, const Map& map, const int viewportWidth, const int viewportHeight) {
    Entity& cameraEntity = world.createEntity();

    SDL_FRect cameraView{};
    cameraView.w = static_cast<float>(viewportWidth);
    cameraView.h = static_cast<float>(viewportHeight);

    cameraEntity.addComponent<Camera>(
        cameraView,
        static_cast<float>(map.width * map.tileWidth),
        static_cast<float>(map.height * map.tileHeight)
    );
}

void createPlayerEntity(World& world, const Map& map, const Vector2D& spawnPoint, TextureManager& textureManager) {
    if (!AssetManager::hasAnimation("player")) {
        AssetManager::loadAnimation("player", "assets/animations/wes_overworld.xml");
    }

    Entity& player = world.createEntity();

    auto& transform = player.addComponent<Transform>(spawnPoint, 0.0f, 1.0f);
    player.addComponent<Velocity>(Vector2D(0.0f, 0.0f), 80.0f);
    player.addComponent<GridMovement>(static_cast<float>(map.tileWidth), false, spawnPoint);

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

    constexpr float kBaseDrawHeight = 32.0f;
    constexpr float kDrawScale = 0.8f;
    const float drawHeight = std::round(kBaseDrawHeight * kDrawScale);
    float drawWidth = 16.0f;
    if (src.w > 0.0f && src.h > 0.0f) {
        drawWidth = std::round((src.w / src.h) * drawHeight);
    }
    drawWidth = std::max(1.0f, drawWidth);

    SDL_FRect dst{transform.position.x, transform.position.y, drawWidth, drawHeight};
    auto& sprite = player.addComponent<Sprite>(textureManager.load("assets/characters/wes/wes_overworld.png"), src, dst);
    const float verticalOffset = std::round(-(drawHeight - static_cast<float>(map.tileHeight)));
    sprite.offset = Vector2D((static_cast<float>(map.tileWidth) - drawWidth) * 0.5f, verticalOffset);
    sprite.mirrorOnRightClip = true;

    auto& collider = player.addComponent<Collider>();
    collider.tag = "player";
    collider.rect.w = static_cast<float>(map.tileWidth);
    collider.rect.h = static_cast<float>(map.tileHeight);
    collider.rect.x = transform.position.x;
    collider.rect.y = transform.position.y;

    player.addComponent<PlayerTag>();
}

void resolvePlayerWallCollisions(World& world) {
    for (const CollisionPair& pair : world.collisions()) {
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
} // namespace OverworldEntityFactory
