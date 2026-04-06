#include "Scene.h"
#include "../TextureManager.h"
#include "AssetManager.h"

namespace {
Entity* findPlayer(World& world) {
    for (auto& entity : world.getEntities()) {
        if (entity && entity->hasComponent<PlayerTag>() && entity->hasComponent<Collider>()) {
            return entity.get();
        }
    }

    return nullptr;
}
}

Scene::Scene(const char* sceneName, const char* mapPath, const char* tilesetPath, const int windowWidth, const int windowHeight, const SceneSpawnRequest& spawnRequest) : name(sceneName) {

    // Load a map
    // Each scene can point at a different tileset image, so we pass that in
    // here instead of forcing every map to look like Pallet Town.
    world.getMap().load(mapPath, TextureManager::load(tilesetPath));
    for (auto &collider : world.getMap().colliders) {
        auto& e = world.createEntity();
        e.addComponent<Transform>(Vector2D(collider.rect.x, collider.rect.y), 0.0f, 1.0f);
        auto& c = e.addComponent<Collider>("wall");
        c.rect.x = collider.rect.x;
        c.rect.y = collider.rect.y;
        c.rect.w = collider.rect.w;
        c.rect.h = collider.rect.h;
    }

    auto& cam = world.createEntity();
    SDL_FRect camView{};
    camView.w = static_cast<float>(windowWidth);
    camView.h = static_cast<float>(windowHeight);
    cam.addComponent<Camera>(camView, world.getMap().width * 32.0f, world.getMap().height * 32.0f);

    Entity& player = world.createEntity();
    // This now comes from the map's spawn points so doorways and route exits
    // can place the player without editing C++ each time.
    const Vector2D playerStart = waypointSystem.resolvePlayerStart(world.getMap().spawnPoints, spawnRequest);
    auto& playerTransform = player.addComponent<Transform>(playerStart, 0.0f, 1.0f);
    // Tile movement now keeps all of the player's movement data in one place.
    // These values mean: 32x32 tiles, 120 move speed, and start on the spawn tile.
    player.addComponent<GridMovement>(32.0f, 120.0f, playerStart);

    Animation anim = AssetManager::getAnimation("player");
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
    SDL_FRect playerDst{ playerTransform.position.x, playerTransform.position.y, playerSrc.w, playerSrc.h };

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
    // This tells the waypoint helper whether the player started on top of a
    // warp tile, which stops a fresh map load from instantly bouncing back.
    waypointSystem.beginScene(playerCollider.rect, world.getMap().warps);
}

void Scene::update(const float dt, SDL_Event &e) {
    world.update(dt, e);

    if (Entity* player = findPlayer(world)) {
        const auto& playerCollider = player->getComponent<Collider>();
        // The simple waypoint pass only needs the player's box to know when a
        // touched warp should request the next map.
        waypointSystem.update(playerCollider.rect, world.getMap().warps, pendingSceneChange);
    }
}

bool Scene::takePendingSceneChange(SceneChangeRequest& request) {
    if (pendingSceneChange.sceneName.empty()) {
        return false;
    }

    request = pendingSceneChange;
    pendingSceneChange = {};
    return true;
}
