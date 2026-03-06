#include "game/world/OverworldEntityFactory.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <string>

#include "engine/ecs/Component.h"
#include "engine/manager/AssetManager.h"
#include "game/world/OverworldCharacterAssets.h"

namespace {
std::string normalizeFacing(std::string facing) {
    std::transform(facing.begin(), facing.end(), facing.begin(), [](const unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    if (facing != "up" && facing != "left" && facing != "right") {
        return "down";
    }

    return facing;
}

const OverworldCharacterAssets& overworldCharacterAssets() {
    static const OverworldCharacterAssets assets = []() {
        OverworldCharacterAssets loaded = makeDefaultOverworldCharacterAssets();
        std::string error;
        if (!loadOverworldCharacterAssetsFromLuaFile("assets/config/world/character_assets.lua", loaded, error) &&
            !error.empty()) {
            std::cout << "Overworld character config load failed; using defaults: " << error << '\n';
        }
        return loaded;
    }();
    return assets;
}
} // namespace

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

void createNpcEntities(World& world, const Map& map, TextureManager& textureManager) {
    if (map.npcSpawns.empty()) {
        return;
    }

    const OverworldCharacterAssets& assets = overworldCharacterAssets();
    const std::string& defaultAnimationPath = assets.npcDefaultAnimationPath;
    const std::string& defaultSpritePath = assets.npcDefaultSpritePath;
    const std::string& npcAnimationKey = assets.npcAnimationKey;

    if (!AssetManager::hasAnimation(npcAnimationKey)) {
        AssetManager::loadAnimation(npcAnimationKey, defaultAnimationPath.c_str());
    }

    const Animation npcAnimation = AssetManager::getAnimation(npcAnimationKey);
    SDL_Texture* defaultNpcTexture = textureManager.load(defaultSpritePath);

    for (const NpcSpawn& npcSpawn : map.npcSpawns) {
        Entity& npc = world.createEntity();

        const SDL_FRect npcRect = npcSpawn.area;
        auto& transform = npc.addComponent<Transform>(Vector2D(npcRect.x, npcRect.y), 0.0f, 1.0f);
        npc.addComponent<Velocity>(Vector2D(0.0f, 0.0f), 0.0f);

        auto& animation = npc.addComponent<Animation>(npcAnimation);
        animation.currentClip = "idle_" + normalizeFacing(npcSpawn.facing);
        animation.baseSpeed = 0.2f;
        animation.speed = animation.baseSpeed;
        animation.currentFrame = 0;
        animation.time = 0.0f;

        SDL_FRect src{0.0f, 0.0f, 16.0f, 32.0f};
        const auto clipIt = animation.clips.find(animation.currentClip);
        if (clipIt != animation.clips.end() && !clipIt->second.frameIndices.empty()) {
            src = clipIt->second.frameIndices.front();
        }

        const float drawWidth = std::max(1.0f, src.w);
        const float drawHeight = std::max(1.0f, src.h);
        SDL_FRect dst{transform.position.x, transform.position.y, drawWidth, drawHeight};

        SDL_Texture* npcTexture = defaultNpcTexture;
        if (!npcSpawn.spritePath.empty()) {
            npcTexture = textureManager.load(npcSpawn.spritePath.c_str());
        }

        auto& sprite = npc.addComponent<Sprite>(npcTexture, src, dst);
        const float verticalOffset = std::round(-(drawHeight - static_cast<float>(map.tileHeight)));
        sprite.offset = Vector2D((static_cast<float>(map.tileWidth) - drawWidth) * 0.5f, verticalOffset);

        auto& collider = npc.addComponent<Collider>();
        collider.tag = "npc";
        collider.rect.x = transform.position.x;
        collider.rect.y = transform.position.y;
        collider.rect.w = npcRect.w > 0.0f ? npcRect.w : static_cast<float>(map.tileWidth);
        collider.rect.h = npcRect.h > 0.0f ? npcRect.h : static_cast<float>(map.tileHeight);

        auto& interaction = npc.addComponent<NpcInteraction>();
        interaction.id = npcSpawn.id;
        interaction.scriptId = npcSpawn.scriptId;
        interaction.speaker = npcSpawn.speaker;
        interaction.dialogue = npcSpawn.dialogue;

        npc.addComponent<NpcTag>();
    }
}

void createPlayerEntity(World& world, const Map& map, const Vector2D& spawnPoint, TextureManager& textureManager) {
    const OverworldCharacterAssets& assets = overworldCharacterAssets();

    if (!AssetManager::hasAnimation("player")) {
        AssetManager::loadAnimation("player", assets.playerAnimationPath.c_str());
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

    const float drawHeight = std::round(assets.playerBaseDrawHeight * assets.playerDrawScale);
    float drawWidth = 16.0f;
    if (src.w > 0.0f && src.h > 0.0f) {
        drawWidth = std::round((src.w / src.h) * drawHeight);
    }
    drawWidth = std::max(1.0f, drawWidth);

    SDL_FRect dst{transform.position.x, transform.position.y, drawWidth, drawHeight};
    auto& sprite = player.addComponent<Sprite>(textureManager.load(assets.playerSpritePath), src, dst);
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
        const bool bBlocksPlayer = colliderB.tag == "wall" || colliderB.tag == "npc";
        const bool aBlocksPlayer = colliderA.tag == "wall" || colliderA.tag == "npc";

        if (colliderA.tag == "player" && bBlocksPlayer) {
            player = pair.entityA;
        } else if (aBlocksPlayer && colliderB.tag == "player") {
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
