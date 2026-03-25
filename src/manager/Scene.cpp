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
    auto& playerTransform = player.addComponent<Transform>(Vector2D(0.0f, 0.0f), 0.0f, 1.0f);
    player.addComponent<Velocity>(Vector2D(0.0f, 0.0f), 120.0f);

    Animation anim = AssetManager::getAnimation("player");
    player.addComponent<Animation>(anim);

    SDL_Texture* tex = TextureManager::load("assets/characters/wes/wes_overworld_updated.png");
    SDL_FRect playerSrc{0.0f, 0.0f, 32.0f, 32.0f};
    auto playerClipIt = anim.clips.find(anim.currentClip);
    if (playerClipIt != anim.clips.end() && !playerClipIt->second.frameIndices.empty()) {
        playerSrc = playerClipIt->second.frameIndices[0];
    }
    SDL_FRect playerDst{ playerTransform.position.x, playerTransform.position.y, 64.0f, 64.0f };

    player.addComponent<Sprite>(tex, playerSrc, playerDst);

    auto& playerCollider = player.addComponent<Collider>("player");
    playerCollider.rect.w = playerDst.w;
    playerCollider.rect.h = playerDst.h;

    player.addComponent<PlayerTag>();

    auto& spawner(world.createEntity());
    Transform t = spawner.addComponent<Transform>(Vector2D(windowWidth / 2.0f, windowHeight - 5.0f), 0.0f, 1.0f);
    spawner.addComponent<TimedSpawner>(2.0f, [this, t] {
        auto& e(world.createDeferredEntity());
        e.addComponent<Transform>(Vector2D(t.position.x, t.position.y), 0.0f, 1.0f);
        e.addComponent<Velocity>(Vector2D(0, -1), 100.0f);

        Animation anim = AssetManager::getAnimation("enemy");
        e.addComponent<Animation>(anim);

        SDL_Texture* tex = TextureManager::load("assets/animations/bird_anim.png");
        SDL_FRect src {0, 0, 32, 32};
        SDL_FRect dest {t.position.x, t.position.y, 32, 32};
        e.addComponent<Sprite>(tex, src, dest);

        Collider c = e.addComponent<Collider>("projectile");
        c.rect.w = dest.w;
        c.rect.h = dest.h;

        e.addComponent<ProjectileTag>();
    });

    auto &state (world.createEntity());
    state.addComponent<SceneState>();
}
