#include "Scene.h"
#include "../Game.h"
#include "SceneSetup.h"
#include "SceneRuntimeHelpers.h"
#include "../ecs/systems/EncounterSystem.h"
#include "../ecs/systems/InteractionSystem.h"
#include <SDL3/SDL_log.h>

extern Game* game;

Scene::Scene(const char* sceneName, const char* mapPath, const char* tilesetPath, const int windowWidth, const int windowHeight, const SceneSpawnRequest& spawnRequest) : name(sceneName) {
    // Scene construction now lives in a setup helper so this file can stay
    // focused on input, encounters, menus, and other per-frame behavior.
    SceneSetup::initialize(
        world,
        dialogueSystem,
        startMenuSystem,
        waypointSystem,
        mapPath,
        tilesetPath,
        windowWidth,
        windowHeight,
        spawnRequest
    );
}

void Scene::update(const float dt, SDL_Event &e) {
    if (startMenuSystem.isOpen()) {
        // While the start menu is open, it owns the controls so the player
        // cannot keep walking around underneath the menu overlay.
        if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
            if (const char* selectedEntry = startMenuSystem.handleKey(e.key.key)) {
                dialogueSystem.openMessage("START MENU", std::string(selectedEntry) + " is not ready yet.");
            }
        }
        return;
    }

    if (dialogueSystem.isOpen()) {
        // While a dialogue box is open, the same confirm key simply closes it
        // so the player cannot keep moving around underneath the text.
        if (SceneRuntimeHelpers::isInteractionKey(e)) {
            dialogueSystem.close();
        }
        return;
    }

    Entity* playerBeforeUpdate = SceneRuntimeHelpers::findPlayer(world);
    Vector2D previousStepDirection{};
    if (playerBeforeUpdate) {
        // The menu opens most cleanly between tile steps, so we only allow it
        // while the player is standing still on a tile.
        auto& playerMovement = playerBeforeUpdate->getComponent<GridMovement>();
        if (SceneRuntimeHelpers::isStartMenuKey(e) && playerMovement.stepDirection == Vector2D(0.0f, 0.0f)) {
            // Opening the menu pauses overworld updates, so we clear held move
            // input here to stop the player from drifting when it closes.
            playerMovement.inputDirection = Vector2D(0.0f, 0.0f);
            startMenuSystem.openMenu();
            return;
        }

        previousStepDirection = playerBeforeUpdate->getComponent<GridMovement>().stepDirection;
    }

    world.update(dt, e);

    if (Entity* player = SceneRuntimeHelpers::findPlayer(world)) {
        const auto& playerCollider = player->getComponent<Collider>();
        auto& playerMovement = player->getComponent<GridMovement>();
        const auto& playerAnimation = player->getComponent<Animation>();
        const Vector2D playerFacing = SceneRuntimeHelpers::facingForClip(playerAnimation.currentClip);
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

        if (SceneRuntimeHelpers::isInteractionKey(e) && playerMovement.stepDirection == Vector2D(0.0f, 0.0f)) {
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
    startMenuSystem.render(game ? game->renderer : nullptr);
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
