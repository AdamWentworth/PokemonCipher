#include "game/OverworldScene.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <utility>

#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_render.h>

#include "engine/ecs/Component.h"
#include "engine/utils/Collision.h"
#include "game/scripting/LuaOverworldScriptLoader.h"
#include "game/world/OverworldEntityFactory.h"

bool OverworldScene::runScript(const std::string& scriptId) {
    const std::string scriptKey = normalizeMapKey(scriptId);
    const std::filesystem::path luaScriptPath = std::filesystem::path("assets") / "scripts" / (scriptKey + ".lua");

    if (scriptKey == "oak_lab_eevee") {
        constexpr int kEeveeSpeciesId = 133;
        constexpr int kStarterLevel = 5;

        const bool hasPartnerEevee = std::any_of(
            gameState_.party().begin(),
            gameState_.party().end(),
            [](const PartyPokemon& member) {
                return member.speciesId == kEeveeSpeciesId && member.isPartner;
            }
        );

        bool alreadyObtainedStarter = gameState_.getFlag("starter_eevee_obtained");
        if (!alreadyObtainedStarter && hasPartnerEevee) {
            gameState_.setFlag("starter_eevee_obtained", true);
            alreadyObtainedStarter = true;
        }

        OverworldScript script{};
        script.id = scriptKey;
        script.commands.push_back(OverworldScriptCommand::LockInput());

        if (!alreadyObtainedStarter) {
            script.commands.push_back(OverworldScriptCommand::Dialogue(
                "PROF. OAK",
                "There you are, ADAM. This EEVEE is counting on you."
            ));
            script.commands.push_back(OverworldScriptCommand::Dialogue(
                "PROF. OAK",
                "Treat your partner with care and grow stronger together."
            ));
            script.commands.push_back(OverworldScriptCommand::AddPartyPokemon(kEeveeSpeciesId, kStarterLevel, true));
            script.commands.push_back(OverworldScriptCommand::SetFlag("starter_eevee_obtained", true));
            script.commands.push_back(OverworldScriptCommand::SetVar("partner_species_id", kEeveeSpeciesId));
            script.commands.push_back(OverworldScriptCommand::SetVar("partner_level", kStarterLevel));
        } else {
            script.commands.push_back(OverworldScriptCommand::Dialogue(
                "PROF. OAK",
                "How is your EEVEE doing? Trust your partner and keep moving forward."
            ));
        }

        script.commands.push_back(OverworldScriptCommand::UnlockInput());
        return startTransientScript(std::move(script));
    }

    if (std::filesystem::exists(luaScriptPath)) {
        OverworldScript loadedScript{};
        std::string loadError;
        if (!loadOverworldScriptFromLuaFile(scriptKey, luaScriptPath.string(), loadedScript, loadError)) {
            printConsole("Lua load error for '" + scriptKey + "': " + loadError);
            return false;
        }

        scripts_[scriptKey] = std::move(loadedScript);
    } else if (scripts_.find(scriptKey) == scripts_.end()) {
        return false;
    }

    const auto it = scripts_.find(scriptKey);
    if (it == scripts_.end()) {
        return false;
    }

    scriptRunner_.stop();
    dialogueOverlay_.hide();
    startMenuOverlay_.close();
    startMenuOverlay_.clearStatusText();
    partyMenuOverlay_.close();
    scriptAdvanceRequested_ = false;
    setScriptInputEnabled(true);

    if (!scriptRunner_.start(&it->second)) {
        return false;
    }

    printConsole("Running script: " + it->second.id);
    return true;
}

bool OverworldScene::loadMap(
    const std::string& mapId,
    const std::string& spawnId,
    const std::optional<Vector2D>& spawnOverride
) {
    const std::string normalizedMapId = normalizeMapKey(mapId);
    const MapDefinition* mapDefinition = mapRegistry_.find(normalizedMapId);
    if (!mapDefinition) {
        std::cout << "Map id '" << mapId << "' is not registered.\n";
        return false;
    }

    if (!map_.load(mapDefinition->mapPath.c_str())) {
        std::cout << "Failed to load map path: " << mapDefinition->mapPath << '\n';
        return false;
    }

    currentMapId_ = mapDefinition->id;
    tilemapRenderer_.setTilesets(
        mapDefinition->baseTilesetPath.c_str(),
        mapDefinition->coverTilesetPath.empty() ? nullptr : mapDefinition->coverTilesetPath.c_str()
    );
    if (!tallGrassRustleTexture_) {
        tallGrassRustleTexture_ = textureManager_.load("assets/effects/tall_grass_rustle.png");
    }
    tallGrassRustles_.clear();

    world_.clear();

    gridMovementSystem_.setWorldBounds(
        static_cast<float>(map_.width * map_.tileWidth),
        static_cast<float>(map_.height * map_.tileHeight)
    );

    OverworldEntityFactory::createStaticCollisionEntities(world_, map_);
    OverworldEntityFactory::createCameraEntity(world_, map_, viewportWidth_, viewportHeight_);
    OverworldEntityFactory::createNpcEntities(world_, map_, textureManager_);

    const Vector2D spawnPoint = spawnOverride.has_value() ? *spawnOverride : map_.getSpawnPoint(spawnId);
    OverworldEntityFactory::createPlayerEntity(world_, map_, spawnPoint, textureManager_);
    refreshInputState();

    warpCooldownSeconds_ = 0.2f;
    encounterCooldownSeconds_ = 0.25f;
    syncEncounterTrackingToPlayer();
    return true;
}

void OverworldScene::queueMapWarp(
    std::string targetMap,
    std::string targetSpawnId,
    std::optional<Vector2D> targetSpawnOverride,
    Vector2D entryDirection
) {
    if (isWarpTransitionActive()) {
        return;
    }

    if (targetSpawnId.empty()) {
        targetSpawnId = "default";
    }

    pendingMapWarp_ = PendingMapWarp{
        std::move(targetMap),
        std::move(targetSpawnId),
        targetSpawnOverride,
        entryDirection
    };
    warpTransitionPhase_ = WarpTransitionPhase::FadeOut;
    warpTransitionTimerSeconds_ = 0.0f;
    refreshInputState();
}

void OverworldScene::applyPlayerFacing(const Vector2D facingDirection) {
    Entity* player = world_.findFirstWith<PlayerTag>();
    if (!player || !player->hasComponent<Animation>()) {
        return;
    }

    auto& animation = player->getComponent<Animation>();
    const float absX = std::abs(facingDirection.x);
    const float absY = std::abs(facingDirection.y);

    std::string clip = "idle_down";
    if (absX > 0.01f || absY > 0.01f) {
        if (absX >= absY) {
            clip = facingDirection.x >= 0.0f ? "idle_right" : "idle_left";
        } else {
            clip = facingDirection.y >= 0.0f ? "idle_down" : "idle_up";
        }
    }

    if (const auto clipIt = animation.clips.find(clip); clipIt != animation.clips.end() && !clipIt->second.frameIndices.empty()) {
        animation.currentClip = clip;
    } else if (!animation.currentClip.empty()) {
        // Keep current clip if desired idle clip is missing.
    } else if (const auto fallbackIt = animation.clips.find("idle_down");
               fallbackIt != animation.clips.end() && !fallbackIt->second.frameIndices.empty()) {
        animation.currentClip = "idle_down";
    } else if (!animation.clips.empty()) {
        animation.currentClip = animation.clips.begin()->first;
    }

    animation.currentFrame = 0;
    animation.time = 0.0f;
}

void OverworldScene::updateWarpTransition(const float dt) {
    if (!isWarpTransitionActive()) {
        return;
    }

    warpTransitionTimerSeconds_ += std::max(0.0f, dt);
    const float duration = std::max(0.01f, warpTransitionDurationSeconds_);
    const float phaseProgress = std::min(1.0f, warpTransitionTimerSeconds_ / duration);

    if (warpTransitionPhase_ == WarpTransitionPhase::FadeOut) {
        if (phaseProgress < 1.0f) {
            return;
        }

        if (pendingMapWarp_) {
            const PendingMapWarp warp = *pendingMapWarp_;
            if (!loadMap(warp.targetMap, warp.targetSpawnId, warp.targetSpawnOverride)) {
                std::cout << "Warp target map not available: " << warp.targetMap << '\n';
            } else {
                applyPlayerFacing(warp.entryDirection);
                requireWarpTileExit_ = true;
                warpEntryDirection_ = warp.entryDirection;
            }
        }

        pendingMapWarp_.reset();
        warpTransitionPhase_ = WarpTransitionPhase::FadeIn;
        warpTransitionTimerSeconds_ = 0.0f;
        return;
    }

    if (warpTransitionPhase_ == WarpTransitionPhase::FadeIn && phaseProgress >= 1.0f) {
        warpTransitionPhase_ = WarpTransitionPhase::None;
        warpTransitionTimerSeconds_ = 0.0f;
        refreshInputState();
    }
}

void OverworldScene::renderWarpTransitionOverlay() const {
    if (!isWarpTransitionActive()) {
        return;
    }

    SDL_Renderer* const renderer = textureManager_.renderer();
    if (!renderer) {
        return;
    }

    const float duration = std::max(0.01f, warpTransitionDurationSeconds_);
    const float phaseProgress = std::min(1.0f, warpTransitionTimerSeconds_ / duration);
    float alpha01 = 0.0f;
    if (warpTransitionPhase_ == WarpTransitionPhase::FadeOut) {
        alpha01 = phaseProgress;
    } else if (warpTransitionPhase_ == WarpTransitionPhase::FadeIn) {
        alpha01 = 1.0f - phaseProgress;
    }

    const Uint8 alpha = static_cast<Uint8>(std::clamp(alpha01, 0.0f, 1.0f) * 255.0f);
    if (alpha == 0) {
        return;
    }

    SDL_FRect fadeRect{
        0.0f,
        0.0f,
        static_cast<float>(viewportWidth_),
        static_cast<float>(viewportHeight_)
    };
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha);
    SDL_RenderFillRect(renderer, &fadeRect);
}

void OverworldScene::checkMapWarps(const float dt) {
    if (scriptRunner_.isRunning() || startMenuOverlay_.isOpen() || partyMenuOverlay_.isOpen() ||
        pokemonSummaryOverlay_.isOpen() || isWarpTransitionActive()) {
        return;
    }

    if (warpCooldownSeconds_ > 0.0f) {
        warpCooldownSeconds_ = std::max(0.0f, warpCooldownSeconds_ - dt);
        return;
    }

    Entity* player = world_.findFirstWith<PlayerTag>();
    if (!player || !player->hasComponent<Collider>()) {
        return;
    }

    const auto& playerCollider = player->getComponent<Collider>().rect;
    auto readRequestedDirection = []() {
        const bool* keyboardState = SDL_GetKeyboardState(nullptr);
        if (!keyboardState) {
            return Vector2D(0.0f, 0.0f);
        }

        if (keyboardState[SDL_SCANCODE_W] || keyboardState[SDL_SCANCODE_UP]) {
            return Vector2D(0.0f, -1.0f);
        }
        if (keyboardState[SDL_SCANCODE_S] || keyboardState[SDL_SCANCODE_DOWN]) {
            return Vector2D(0.0f, 1.0f);
        }
        if (keyboardState[SDL_SCANCODE_A] || keyboardState[SDL_SCANCODE_LEFT]) {
            return Vector2D(-1.0f, 0.0f);
        }
        if (keyboardState[SDL_SCANCODE_D] || keyboardState[SDL_SCANCODE_RIGHT]) {
            return Vector2D(1.0f, 0.0f);
        }

        return Vector2D(0.0f, 0.0f);
    };

    if (requireWarpTileExit_) {
        const bool standingOnWarp = std::any_of(map_.warpPoints.begin(), map_.warpPoints.end(), [&playerCollider](const WarpPoint& warp) {
            return Collision::AABB(playerCollider, warp.area);
        });

        if (standingOnWarp) {
            const Vector2D requestedDirection = readRequestedDirection();

            const bool hasDirectionalInput = std::abs(requestedDirection.x) > 0.01f || std::abs(requestedDirection.y) > 0.01f;
            if (!hasDirectionalInput) {
                return;
            }

            const float previousDirMagnitude = std::abs(warpEntryDirection_.x) + std::abs(warpEntryDirection_.y);
            if (previousDirMagnitude > 0.01f) {
                const float dot = (requestedDirection.x * warpEntryDirection_.x) + (requestedDirection.y * warpEntryDirection_.y);
                if (dot > -0.5f) {
                    return;
                }
            }
        }

        requireWarpTileExit_ = false;
        warpEntryDirection_ = Vector2D(0.0f, 0.0f);
    }

    for (const WarpPoint& warp : map_.warpPoints) {
        if (!Collision::AABB(playerCollider, warp.area)) {
            continue;
        }

        if (!warp.requiredDirection.empty()) {
            const Vector2D requestedDirection = readRequestedDirection();
            const float absX = std::abs(requestedDirection.x);
            const float absY = std::abs(requestedDirection.y);

            bool matchesRequiredDirection = false;
            if (warp.requiredDirection == "left") {
                matchesRequiredDirection = requestedDirection.x < -0.01f && absX >= absY;
            } else if (warp.requiredDirection == "right") {
                matchesRequiredDirection = requestedDirection.x > 0.01f && absX >= absY;
            } else if (warp.requiredDirection == "up") {
                matchesRequiredDirection = requestedDirection.y < -0.01f && absY >= absX;
            } else if (warp.requiredDirection == "down") {
                matchesRequiredDirection = requestedDirection.y > 0.01f && absY >= absX;
            }

            if (!matchesRequiredDirection) {
                continue;
            }
        }

        const float worldWidth = static_cast<float>(map_.width * map_.tileWidth);
        const float worldHeight = static_cast<float>(map_.height * map_.tileHeight);
        const bool isSingleTileWarp =
            warp.area.w <= static_cast<float>(map_.tileWidth) + 0.01f &&
            warp.area.h <= static_cast<float>(map_.tileHeight) + 0.01f;
        const bool touchesMapEdge =
            warp.area.x <= 0.01f ||
            warp.area.y <= 0.01f ||
            (warp.area.x + warp.area.w) >= (worldWidth - 0.01f) ||
            (warp.area.y + warp.area.h) >= (worldHeight - 0.01f);
        const bool isInteriorWarp = isSingleTileWarp && !touchesMapEdge;

        if (isInteriorWarp) {
            if (player->hasComponent<GridMovement>() && player->getComponent<GridMovement>().isMoving) {
                // Stair-style warps should trigger only after the step onto the stair tile finishes.
                continue;
            }
        }

        std::string targetMap = normalizeMapKey(warp.targetMap);
        if (targetMap.rfind("map_", 0) == 0) {
            targetMap = targetMap.substr(4);
        }

        Vector2D entryDirection = playerFacingDirection();
        if (std::abs(entryDirection.x) < 0.01f && std::abs(entryDirection.y) < 0.01f) {
            entryDirection = Vector2D(0.0f, 1.0f);
        }

        std::string targetSpawnId = warp.targetSpawnId.empty() ? "default" : warp.targetSpawnId;
        std::optional<Vector2D> targetSpawnOverride;
        if (warp.hasTargetSpawn) {
            targetSpawnOverride = warp.targetSpawn;
        }
        queueMapWarp(
            std::move(targetMap),
            std::move(targetSpawnId),
            targetSpawnOverride,
            entryDirection
        );
        return;
    }
}

std::string OverworldScene::normalizeMapKey(std::string key) {
    std::transform(key.begin(), key.end(), key.begin(), [](const unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return key;
}

void OverworldScene::registerDefaultScripts() {
    scripts_.clear();
}
