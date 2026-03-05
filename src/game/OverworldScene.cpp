#include "game/OverworldScene.h"

#include <algorithm>
#include <iostream>

#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>

#include "engine/ecs/Component.h"
#include "game/world/OverworldEntityFactory.h"

OverworldScene::OverworldScene(
    TextureManager& textureManager,
    GameState& gameState,
    const MapRegistry& mapRegistry,
    const std::string& initialMapId,
    const int viewportWidth,
    const int viewportHeight
) : textureManager_(textureManager),
    gameState_(gameState),
    mapRegistry_(mapRegistry),
    viewportWidth_(viewportWidth),
    viewportHeight_(viewportHeight),
    tilemapRenderer_(textureManager, nullptr, nullptr) {
    if (!loadMap(initialMapId, "default")) {
        std::cout << "Failed to load initial map id: " << initialMapId << '\n';
    }

    registerDefaultScripts();
    createDevConsole();
    refreshInputState();
}

void OverworldScene::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat && event.key.key == SDLK_GRAVE) {
        setDebugConsoleOpen(!debugConsoleOpen_);
        return;
    }

    if (!debugConsoleOpen_) {
        if (scriptRunner_.isRunning() && event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
            const SDL_Keycode key = event.key.key;
            if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE || key == SDLK_Z || key == SDLK_X) {
                scriptAdvanceRequested_ = true;
            }
        }
        return;
    }

    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
        if (event.key.key == SDLK_ESCAPE) {
            setDebugConsoleOpen(false);
            return;
        }

        if (event.key.key == SDLK_BACKSPACE) {
            if (!debugConsoleInput_.empty()) {
                debugConsoleInput_.pop_back();
            }
            return;
        }

        if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER) {
            if (devConsole_) {
                devConsole_->execute(debugConsoleInput_);
            }
            debugConsoleInput_.clear();
            return;
        }
    }

    if (event.type == SDL_EVENT_TEXT_INPUT && event.text.text && event.text.text[0] != '\0') {
        debugConsoleInput_ += event.text.text;
    }
}

void OverworldScene::update(const float dt) {
    if (!introScriptChecked_) {
        introScriptChecked_ = true;
        if (!gameState_.getFlag("intro_complete")) {
            if (!runScript("intro_prologue")) {
                printConsole("Intro script not found: intro_prologue.lua");
            }
        }
    }

    OverworldScriptRunner::Runtime scriptRuntime{
        gameState_,
        [this](const std::string& message) {
            printConsole("[script] " + message);
        },
        [this](const std::string& speaker, const std::string& text) {
            dialogueOverlay_.show(speaker, text);
        },
        [this]() {
            dialogueOverlay_.hide();
        },
        [this]() {
            return consumeScriptAdvanceRequested();
        },
        [this](const bool isInputEnabled) {
            setScriptInputEnabled(isInputEnabled);
        },
        [this](const std::string& mapId, const std::string& spawnId) {
            return warpTo(mapId, spawnId);
        },
        [this](const std::string& mapId, const Vector2D& spawnPoint) {
            return warpTo(mapId, spawnPoint);
        },
        [this](const Vector2D& position) {
            return teleportPlayer(position);
        }
    };
    scriptRunner_.update(dt, scriptRuntime);

    gridMovementSystem_.update(world_.entities(), dt);
    world_.update(dt);
    OverworldEntityFactory::resolvePlayerWallCollisions(world_);
    checkMapWarps(dt);
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

    dialogueOverlay_.render(textureManager_.renderer(), viewportWidth_, viewportHeight_);
}

bool OverworldScene::warpTo(const std::string& mapId, const std::string& spawnId) {
    return loadMap(mapId, spawnId);
}

bool OverworldScene::warpTo(const std::string& mapId, const Vector2D& spawnPoint) {
    return loadMap(mapId, "default", spawnPoint);
}

bool OverworldScene::teleportPlayer(const Vector2D& pixelPosition) {
    Entity* player = world_.findFirstWith<PlayerTag>();
    if (!player || !player->hasComponent<Transform>()) {
        return false;
    }

    auto& transform = player->getComponent<Transform>();

    float clampedX = pixelPosition.x;
    float clampedY = pixelPosition.y;

    if (player->hasComponent<Collider>()) {
        const auto& collider = player->getComponent<Collider>().rect;
        const float maxX = std::max(0.0f, static_cast<float>(map_.width * map_.tileWidth) - collider.w);
        const float maxY = std::max(0.0f, static_cast<float>(map_.height * map_.tileHeight) - collider.h);
        clampedX = std::clamp(clampedX, 0.0f, maxX);
        clampedY = std::clamp(clampedY, 0.0f, maxY);
    }

    transform.position = Vector2D(clampedX, clampedY);
    transform.oldPosition = transform.position;

    if (player->hasComponent<Velocity>()) {
        auto& velocity = player->getComponent<Velocity>();
        velocity.direction = Vector2D(0.0f, 0.0f);
    }

    if (player->hasComponent<GridMovement>()) {
        auto& gridMovement = player->getComponent<GridMovement>();
        gridMovement.isMoving = false;
        gridMovement.targetPosition = transform.position;
        gridMovement.stepDirection = Vector2D(0.0f, 0.0f);
        gridMovement.bumpTimeRemaining = 0.0f;
        gridMovement.bumpDirection = Vector2D(0.0f, 0.0f);
        gridMovement.applyWalkStartFrame = false;
    }

    return true;
}
