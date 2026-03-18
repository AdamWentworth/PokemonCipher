#include "Scene.h"
#include "../TextureManager.h"
#include "AssetManager.h"

Scene::Scene(const char* sceneName, const char* mapPath, const int windowWidth, const int windowHeight)
    : name(sceneName) {
    SDL_Texture* mapTexture = TextureManager::load("assets/tilesets/pallet_town/pallet_town_tileset.png");
    world.getMap().load(mapPath, mapTexture);

    for (const auto& collider : world.getMap().colliders) {
        auto& wall = world.createEntity();
        wall.addComponent<Transform>(Vector2D(collider.rect.x, collider.rect.y), 0.0f, 1.0f);
        auto& wallCollider = wall.addComponent<Collider>("wall");
        wallCollider.rect = collider.rect;
    }

    auto& cam = world.createEntity();
    SDL_FRect camView{};
    camView.w = static_cast<float>(windowWidth);
    camView.h = static_cast<float>(windowHeight);
    cam.addComponent<Camera>(
        camView,
        static_cast<float>(world.getMap().width * world.getMap().tileWidth * world.getMap().renderScale),
        static_cast<float>(world.getMap().height * world.getMap().tileHeight * world.getMap().renderScale));

    auto& player = world.createEntity();
    const Vector2D spawn = world.getMap().getSpawnPoint("default", Vector2D(96.0f, 144.0f));
    auto& playerTransform = player.addComponent<Transform>(spawn, 0.0f, 1.0f);
    player.addComponent<Velocity>(Vector2D(0.0f, 0.0f), 120.0f);

    Animation anim = AssetManager::getAnimation("player");
    anim.currentClip = anim.clips.contains("idle_down") ? "idle_down" : anim.currentClip;
    player.addComponent<Animation>(anim);

    SDL_Texture* playerTexture = TextureManager::load("assets/characters/wes/wes_overworld.png");
    SDL_FRect playerSrc{0.0f, 0.0f, 31.0f, 45.0f};
    if (const auto clipIt = anim.clips.find(anim.currentClip);
        clipIt != anim.clips.end() && !clipIt->second.frameIndices.empty()) {
        playerSrc = clipIt->second.frameIndices.front();
    }

    SDL_FRect playerDst{playerTransform.position.x, playerTransform.position.y, 62.0f, 90.0f};
    player.addComponent<Sprite>(playerTexture, playerSrc, playerDst);

    auto& playerCollider = player.addComponent<Collider>("player");
    playerCollider.rect.x = playerTransform.position.x + 18.0f;
    playerCollider.rect.y = playerTransform.position.y + 58.0f;
    playerCollider.rect.w = 24.0f;
    playerCollider.rect.h = 28.0f;

    player.addComponent<PlayerTag>();

    auto& state = world.createEntity();
    state.addComponent<SceneState>();
}
