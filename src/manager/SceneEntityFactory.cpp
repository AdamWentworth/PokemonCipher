#include "SceneEntityFactory.h"

#include "../TextureManager.h"
#include "AssetManager.h"

namespace {
constexpr const char* kOakInteractionId = "oak_lab_eevee";
constexpr const char* kOakAnimationId = "oak_npc";

std::string idleClipForFacing(const Vector2D& facingDirection) {
    if (facingDirection.x > 0.0f) return "idle_right";
    if (facingDirection.x < 0.0f) return "idle_left";
    if (facingDirection.y < 0.0f) return "idle_up";
    return "idle_down";
}

void createOakVisual(World& world, const InteractionPoint& interaction) {
    Entity& oak = world.createEntity();
    oak.addComponent<Transform>(Vector2D(interaction.rect.x, interaction.rect.y), 0.0f, 1.0f);

    // Oak only needs one idle pose for this first dialogue pass, so we can
    // build him from a fixed asset pair instead of making interactions carry
    // around full sprite setup data.
    AssetManager::loadAnimation(kOakAnimationId, "assets/animations/red_overworld.xml");
    Animation anim = AssetManager::getAnimation(kOakAnimationId);
    anim.currentClip = "idle_down";
    oak.addComponent<Animation>(anim);

    SDL_FRect src{0.0f, 0.0f, 16.0f, 32.0f};
    auto clipIt = anim.clips.find(anim.currentClip);
    if (clipIt != anim.clips.end() && !clipIt->second.frameIndices.empty()) {
        src = clipIt->second.frameIndices[0];
    }

    SDL_FRect dst{interaction.rect.x, interaction.rect.y, src.w * 2.0f, src.h * 2.0f};
    auto& sprite = oak.addComponent<Sprite>(TextureManager::load("assets/characters/oak/oak_overworld.png"), src, dst);
    sprite.offset = Vector2D((interaction.rect.w - dst.w) * 0.5f, -(dst.h - interaction.rect.h));

    auto& collider = oak.addComponent<Collider>("wall");
    // Oak should block the same tile he stands on, so using a wall collider
    // lets the existing movement check stop the player without new NPC rules.
    collider.rect = interaction.rect;
}

void addMapColliders(World& world) {
    for (auto& collider : world.getMap().colliders) {
        auto& entity = world.createEntity();
        entity.addComponent<Transform>(Vector2D(collider.rect.x, collider.rect.y), 0.0f, 1.0f);
        auto& wallCollider = entity.addComponent<Collider>("wall");
        wallCollider.rect = collider.rect;
    }
}

void addInteractionVisuals(World& world) {
    for (const auto& interaction : world.getMap().interactions) {
        if (interaction.id == kOakInteractionId) {
            createOakVisual(world, interaction);
        }
    }
}

void createCamera(World& world, const int windowWidth, const int windowHeight) {
    auto& camera = world.createEntity();
    SDL_FRect cameraView{};
    cameraView.w = static_cast<float>(windowWidth);
    cameraView.h = static_cast<float>(windowHeight);
    camera.addComponent<Camera>(cameraView, world.getMap().width * 32.0f, world.getMap().height * 32.0f);
}

void createPlayer(World& world, WaypointSystem& waypointSystem, const SceneSpawnRequest& spawnRequest) {
    Entity& player = world.createEntity();
    // This now comes from the map's spawn points so doorways and route exits
    // can place the player without editing C++ each time.
    const Vector2D playerStart = waypointSystem.resolvePlayerStart(world.getMap().spawnPoints, spawnRequest);
    auto& playerTransform = player.addComponent<Transform>(playerStart, 0.0f, 1.0f);
    // Tile movement now keeps all of the player's movement data in one place.
    // These values mean: 32x32 tiles, 120 move speed, and start on the spawn tile.
    player.addComponent<GridMovement>(32.0f, 120.0f, playerStart);
    // Doorways can carry in a facing direction; if they do not, we fall back
    // to the normal down-facing pose the player had before waypoints existed.
    const Vector2D playerFacing = spawnRequest.facingDirection == Vector2D(0.0f, 0.0f)
        ? Vector2D(0.0f, 1.0f)
        : spawnRequest.facingDirection;

    Animation anim = AssetManager::getAnimation("player");
    // Set the opening idle pose from the carried-in facing so a doorway keeps
    // the player looking the same way on the first frame of the new map.
    anim.currentClip = idleClipForFacing(playerFacing);
    player.addComponent<Animation>(anim);

    SDL_Texture* tex = TextureManager::load("assets/characters/wes/wes_overworld_updated.png");
    // Wes's animation frames are taller than one tile, so we start from his
    // authored frame size and fix the gameplay footprint separately below.
    SDL_FRect playerSrc{0.0f, 0.0f, 31.0f, 45.0f};
    auto playerClipIt = anim.clips.find(anim.currentClip);
    if (playerClipIt != anim.clips.end() && !playerClipIt->second.frameIndices.empty()) {
        playerSrc = playerClipIt->second.frameIndices[0];
    }
    const float playerFootprint = 32.0f;
    // Keep Wes at his authored aspect ratio so he reads like a character sprite
    // instead of being compressed into a square just to fit a one-tile collider.
    SDL_FRect playerDst{playerTransform.position.x, playerTransform.position.y, playerSrc.w, playerSrc.h};

    auto& playerSprite = player.addComponent<Sprite>(tex, playerSrc, playerDst);
    // Move the taller sprite upward so the transform still marks the tile the
    // player is standing on, with the extra height extending above that tile.
    playerSprite.offset = Vector2D((playerFootprint - playerDst.w) * 0.5f, -(playerDst.h - playerFootprint));

    auto& playerCollider = player.addComponent<Collider>("player");
    // Keep collision at exactly one tile even though the sprite is taller,
    // because the goal was a one-tile playable footprint, not a one-tile-tall sprite.
    playerCollider.rect.x = playerStart.x;
    playerCollider.rect.y = playerStart.y;
    playerCollider.rect.w = playerFootprint;
    playerCollider.rect.h = playerFootprint;

    player.addComponent<PlayerTag>();
    // Let the waypoint helper remember whether this spawn started on a doorway
    // tile so the player does not bounce straight back through it.
    waypointSystem.beginScene(playerCollider.rect, world.getMap().warps, playerFacing);
}
}

namespace SceneEntityFactory {
void populate(
    World& world,
    WaypointSystem& waypointSystem,
    const int windowWidth,
    const int windowHeight,
    const SceneSpawnRequest& spawnRequest
) {
    // Once the map data is loaded, this factory turns that data into the
    // actual scene entities the overworld needs to run.
    addMapColliders(world);
    addInteractionVisuals(world);
    createCamera(world, windowWidth, windowHeight);
    createPlayer(world, waypointSystem, spawnRequest);
}
}
