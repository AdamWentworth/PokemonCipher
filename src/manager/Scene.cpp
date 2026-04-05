#include "Scene.h"
#include "../TextureManager.h"
#include "AssetManager.h"

Scene::Scene(const char* sceneName, const char* mapPath, const int windowWidth, const int windowHeight) : name(sceneName) {

    // Load a map
    world.getMap().load(mapPath, TextureManager::load("assets/tilesets/pallet_town/pallet_town_tileset.png"));
    for (auto &collider : world.getMap().colliders) {
        auto& e = world.createEntity();
        e.addComponent<Transform>(Vector2D(collider.rect.x, collider.rect.y), 0.0f, 1.0f);
        auto& c = e.addComponent<Collider>("wall");
        c.rect.x = collider.rect.x;
        c.rect.y = collider.rect.y;
        c.rect.w = collider.rect.w;
        c.rect.h = collider.rect.h;
    }

    // add entities
    // load once and reuse for every coin
    SDL_Texture* itemTex = TextureManager::load("assets/coin.png");
    SDL_FRect itemSrc{ 0.0f, 0.0f, 32.0f, 32.0f };
    // spawn coins from the map point layer
    for (const auto& spawnPoint : world.getMap().itemSpawnPoints) {
        Entity& item = world.createEntity();
        auto& itemTransform = item.addComponent<Transform>(Vector2D(spawnPoint.x, spawnPoint.y), 0.0f, 1.0f);
        SDL_FRect itemDest{ itemTransform.position.x, itemTransform.position.y, 32.0f, 32.0f };

        item.addComponent<Sprite>(itemTex, itemSrc, itemDest);

        auto& itemCollider = item.addComponent<Collider>("item");
        itemCollider.rect.w = itemDest.w;
        itemCollider.rect.h = itemDest.h;
    }

    auto& cam = world.createEntity();
    SDL_FRect camView{};
    camView.w = static_cast<float>(windowWidth);
    camView.h = static_cast<float>(windowHeight);
    cam.addComponent<Camera>(camView, world.getMap().width * 32.0f, world.getMap().height * 32.0f);

    Entity& player = world.createEntity();
    // Use a clear starting tile that does not overlap the scaled wall colliders,
    // because starting inside a wall causes every move to be immediately undone.
    const Vector2D playerStart(192.0f, 288.0f);
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
    playerCollider.rect.w = playerFootprint;
    playerCollider.rect.h = playerFootprint;

    player.addComponent<PlayerTag>();

    auto &state (world.createEntity());
    state.addComponent<SceneState>();
}
