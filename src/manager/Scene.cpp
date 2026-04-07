#include "Scene.h"
#include "../Game.h"
#include "../TextureManager.h"
#include "AssetManager.h"
#include "../ecs/systems/EncounterSystem.h"
#include "../ecs/systems/InteractionSystem.h"
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_log.h>

extern Game* game;

namespace {
constexpr const char* kOakInteractionId = "oak_lab_eevee";
constexpr const char* kOakAnimationId = "oak_npc";

Entity* findPlayer(World& world) {
    for (auto& entity : world.getEntities()) {
        if (entity && entity->hasComponent<PlayerTag>() && entity->hasComponent<Collider>()) {
            return entity.get();
        }
    }

    return nullptr;
}

std::string idleClipForFacing(const Vector2D& facingDirection) {
    if (facingDirection.x > 0.0f) return "idle_right";
    if (facingDirection.x < 0.0f) return "idle_left";
    if (facingDirection.y < 0.0f) return "idle_up";
    return "idle_down";
}

// The current clip name already tells us which way the player is facing, so we
// can reuse that instead of storing a second copy of the same idea in movement.
Vector2D facingForClip(const std::string& clipName) {
    if (clipName.find("right") != std::string::npos) return Vector2D(1.0f, 0.0f);
    if (clipName.find("left") != std::string::npos) return Vector2D(-1.0f, 0.0f);
    if (clipName.find("up") != std::string::npos) return Vector2D(0.0f, -1.0f);
    return Vector2D(0.0f, 1.0f);
}

// These keys match the usual Pokemon-style confirm button so the player can
// reuse the same habit later for signs, NPCs, and dialogue boxes.
bool isInteractionKey(const SDL_Event& event) {
    if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) {
        return false;
    }

    return event.key.key == SDLK_RETURN ||
        event.key.key == SDLK_KP_ENTER ||
        event.key.key == SDLK_SPACE ||
        event.key.key == SDLK_Z;
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
}
}

Scene::Scene(const char* sceneName, const char* mapPath, const char* tilesetPath, const int windowWidth, const int windowHeight, const SceneSpawnRequest& spawnRequest) : name(sceneName) {
    dialogueSystem.setViewportSize(windowWidth, windowHeight);

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

    for (const auto& interaction : world.getMap().interactions) {
        if (interaction.id == kOakInteractionId) {
            createOakVisual(world, interaction);
        }
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
    // Let the waypoint helper remember whether this spawn started on a doorway
    // tile so the player does not bounce straight back through it.
    waypointSystem.beginScene(playerCollider.rect, world.getMap().warps, playerFacing);
}

void Scene::update(const float dt, SDL_Event &e) {
    if (dialogueSystem.isOpen()) {
        // While a dialogue box is open, the same confirm key simply closes it
        // so the player cannot keep moving around underneath the text.
        if (isInteractionKey(e)) {
            dialogueSystem.close();
        }
        return;
    }

    Entity* playerBeforeUpdate = findPlayer(world);
    Vector2D previousStepDirection{};
    if (playerBeforeUpdate) {
        previousStepDirection = playerBeforeUpdate->getComponent<GridMovement>().stepDirection;
    }

    world.update(dt, e);

    if (Entity* player = findPlayer(world)) {
        const auto& playerCollider = player->getComponent<Collider>();
        auto& playerMovement = player->getComponent<GridMovement>();
        const auto& playerAnimation = player->getComponent<Animation>();
        const Vector2D playerFacing = facingForClip(playerAnimation.currentClip);
        const bool justFinishedStep =
            !(previousStepDirection == Vector2D(0.0f, 0.0f)) &&
            playerMovement.stepDirection == Vector2D(0.0f, 0.0f);

        if (justFinishedStep) {
            // Grass should only roll once the player has fully landed on that
            // tile, not while they are still sliding into it mid-step.
            if (const char* encounterText = EncounterSystem{}.tryStartEncounter(
                playerCollider.rect,
                world.getMap().encounterZones
            )) {
                // The encounter box pauses overworld updates, so we clear the
                // last move input here to stop the player from resuming that
                // old direction when the box closes.
                playerMovement.inputDirection = Vector2D(0.0f, 0.0f);
                dialogueSystem.openMessage("WILD ENCOUNTER", encounterText);
                return;
            }
        }

        if (isInteractionKey(e) && playerMovement.stepDirection == Vector2D(0.0f, 0.0f)) {
            // We only check interactions when the player is standing on a tile,
            // so the button talks to the thing in front of them instead of mid-step.
            if (const InteractionPoint* interaction = InteractionSystem{}.findInteraction(
                playerCollider.rect,
                playerFacing,
                playerMovement.tileSize,
                world.getMap().interactions
            )) {
                // Dialogue uses the same pause flow as encounters, so we clear
                // the held move here for the same reason before opening text.
                playerMovement.inputDirection = Vector2D(0.0f, 0.0f);
                if (!dialogueSystem.openForInteraction(*interaction)) {
                    SDL_Log("Interaction: %s", interaction->id.c_str());
                }
            }
        }
        // The waypoint helper turns a touched doorway or route edge into the
        // next scene request. We read facing from the current animation clip so
        // doorway state does not need its own extra movement field.
        waypointSystem.update(
            playerCollider.rect,
            playerMovement.inputDirection,
            playerFacing,
            world.getMap().warps,
            pendingSceneChange
        );
    }
}

void Scene::render() {
    world.render();
    dialogueSystem.render(game ? game->renderer : nullptr);
}

bool Scene::takePendingSceneChange(SceneChangeRequest& request) {
    if (pendingSceneChange.sceneName.empty()) {
        return false;
    }

    request = pendingSceneChange;
    pendingSceneChange = {};
    return true;
}
