#include "game/OverworldScene.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_render.h>

#include "engine/ecs/Component.h"
#include "engine/utils/Collision.h"

void OverworldScene::createDevConsole() {
    OverworldDevConsole::Dependencies dependencies{};
    dependencies.print = [this](const std::string& message) {
        printConsole(message);
    };
    dependencies.listMapIds = [this]() {
        return mapRegistry_.mapIds();
    };
    dependencies.currentMapId = [this]() {
        return currentMapId_;
    };
    dependencies.listScriptIds = [this]() {
        std::vector<std::string> scriptIds;
        scriptIds.reserve(scripts_.size());
        for (const auto& [_, script] : scripts_) {
            scriptIds.push_back(script.id);
        }
        return scriptIds;
    };
    dependencies.listEncounterTableIds = [this]() {
        return wildEncounterService_.tableIds();
    };
    dependencies.runScript = [this](const std::string& scriptId) {
        return runScript(scriptId);
    };
    dependencies.triggerEncounter = [this](const std::string& tableId) {
        if (tableId.empty()) {
            const Entity* player = world_.findFirstWith<PlayerTag>();
            if (!player || !player->hasComponent<Collider>()) {
                return false;
            }

            for (const EncounterZone& zone : map_.encounterZones) {
                if (!Collision::AABB(player->getComponent<Collider>().rect, zone.area)) {
                    continue;
                }

                return triggerWildEncounter(zone.tableId.empty() ? "default_grass" : zone.tableId);
            }

            return false;
        }

        return triggerWildEncounter(tableId);
    };
    dependencies.stopScript = [this]() {
        if (!scriptRunner_.isRunning()) {
            return false;
        }

        scriptRunner_.stop();
        dialogueOverlay_.hide();
        scriptAdvanceRequested_ = false;
        setScriptInputEnabled(true);
        return true;
    };
    dependencies.warpToSpawn = [this](const std::string& mapId, const std::string& spawnId) {
        return warpTo(mapId, spawnId);
    };
    dependencies.warpToPoint = [this](const std::string& mapId, const Vector2D& spawnPoint) {
        return warpTo(mapId, spawnPoint);
    };
    dependencies.teleportToPoint = [this](const Vector2D& position) {
        return teleportPlayer(position);
    };
    dependencies.reloadCurrentMap = [this]() {
        return loadMap(currentMapId_, "default");
    };
    dependencies.listSpawnIds = [this]() {
        std::vector<std::string> spawnIds;
        spawnIds.reserve(map_.spawnPoints.size());
        for (const auto& [id, _] : map_.spawnPoints) {
            spawnIds.push_back(id);
        }

        std::sort(spawnIds.begin(), spawnIds.end());
        return spawnIds;
    };
    dependencies.playerPosition = [this]() -> std::optional<OverworldDevConsole::PlayerPosition> {
        const Entity* player = world_.findFirstWith<PlayerTag>();
        if (!player || !player->hasComponent<Transform>()) {
            return std::nullopt;
        }

        const auto& position = player->getComponent<Transform>().position;
        const int tileX = map_.tileWidth > 0 ? static_cast<int>(position.x / static_cast<float>(map_.tileWidth)) : 0;
        const int tileY = map_.tileHeight > 0 ? static_cast<int>(position.y / static_cast<float>(map_.tileHeight)) : 0;

        return OverworldDevConsole::PlayerPosition{
            position.x,
            position.y,
            tileX,
            tileY
        };
    };
    dependencies.partySummary = [this]() {
        const auto& party = gameState_.party();
        if (party.empty()) {
            return std::string();
        }

        std::ostringstream stream;
        stream << "Party:";
        for (std::size_t i = 0; i < party.size(); ++i) {
            const auto& member = party[i];
            stream << " [" << i
                   << "] species=" << member.speciesId
                   << " lvl=" << member.level;
            if (member.isPartner) {
                stream << " partner";
            }
        }

        return stream.str();
    };
    dependencies.tileToPixel = [this](const int tileX, const int tileY) {
        return Vector2D(
            static_cast<float>(tileX * map_.tileWidth),
            static_cast<float>(tileY * map_.tileHeight)
        );
    };
    dependencies.gameState = &gameState_;

    devConsole_ = std::make_unique<OverworldDevConsole>(std::move(dependencies));
}

void OverworldScene::setScriptInputEnabled(const bool isEnabled) {
    scriptInputLocked_ = !isEnabled;
    refreshInputState();
}

void OverworldScene::refreshInputState() {
    gridMovementSystem_.setInputEnabled(
        !debugConsoleOpen_ &&
        !scriptInputLocked_ &&
        !startMenuOverlay_.isOpen() &&
        !partyMenuOverlay_.isOpen()
    );
}

void OverworldScene::setDebugConsoleOpen(const bool isOpen) {
    if (debugConsoleOpen_ == isOpen) {
        return;
    }

    debugConsoleOpen_ = isOpen;
    if (debugConsoleOpen_) {
        startMenuOverlay_.close();
        startMenuOverlay_.clearStatusText();
        partyMenuOverlay_.close();
    }
    debugConsoleInput_.clear();

    SDL_Window* const keyboardWindow = SDL_GetKeyboardFocus();
    if (keyboardWindow) {
        if (debugConsoleOpen_) {
            SDL_StartTextInput(keyboardWindow);
        } else {
            SDL_StopTextInput(keyboardWindow);
        }
    }

    refreshInputState();

    if (debugConsoleOpen_) {
        printConsole("Dev console open. Use `help` then Enter.");
    } else {
        printConsole("Dev console closed.");
    }
}

void OverworldScene::printConsole(const std::string& message) {
    constexpr std::size_t kMaxHistoryLines = 12;
    debugConsoleHistory_.push_back(message);
    if (debugConsoleHistory_.size() > kMaxHistoryLines) {
        debugConsoleHistory_.pop_front();
    }

    std::cout << "[dev] " << message << '\n';
}

void OverworldScene::renderDevConsoleOverlay() const {
    if (!debugConsoleOpen_) {
        return;
    }

    SDL_Renderer* renderer = textureManager_.renderer();
    if (!renderer) {
        return;
    }

    SDL_FRect overlay{};
    overlay.x = 6.0f;
    overlay.y = 6.0f;
    overlay.w = static_cast<float>(viewportWidth_) - 12.0f;
    overlay.h = static_cast<float>(viewportHeight_) - 12.0f;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 8, 10, 12, 220);
    SDL_RenderFillRect(renderer, &overlay);
    SDL_SetRenderDrawColor(renderer, 232, 232, 232, 255);
    SDL_RenderRect(renderer, &overlay);

    SDL_SetRenderDrawColor(renderer, 255, 231, 128, 255);
    SDL_RenderDebugText(renderer, overlay.x + 8.0f, overlay.y + 8.0f, "DEV CONSOLE");

    SDL_SetRenderDrawColor(renderer, 180, 200, 220, 255);
    SDL_RenderDebugText(renderer, overlay.x + 8.0f, overlay.y + 18.0f, "help | maps | warp | runscript | encounter");

    float lineY = overlay.y + 30.0f;
    const std::size_t visibleHistory = std::min<std::size_t>(8, debugConsoleHistory_.size());
    const std::size_t historyStart = debugConsoleHistory_.size() - visibleHistory;
    for (std::size_t i = 0; i < visibleHistory; ++i) {
        const std::string line = debugConsoleHistory_[historyStart + i];
        SDL_SetRenderDrawColor(renderer, 232, 232, 232, 255);
        SDL_RenderDebugText(renderer, overlay.x + 8.0f, lineY, line.c_str());
        lineY += 10.0f;
    }

    SDL_FRect inputBox{};
    inputBox.x = overlay.x + 6.0f;
    inputBox.h = 14.0f;
    inputBox.w = overlay.w - 12.0f;
    inputBox.y = overlay.y + overlay.h - inputBox.h - 6.0f;

    SDL_SetRenderDrawColor(renderer, 20, 24, 32, 255);
    SDL_RenderFillRect(renderer, &inputBox);
    SDL_SetRenderDrawColor(renderer, 232, 232, 232, 255);
    SDL_RenderRect(renderer, &inputBox);

    const std::string prompt = "> " + debugConsoleInput_;
    SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
    SDL_RenderDebugText(renderer, inputBox.x + 4.0f, inputBox.y + 3.0f, prompt.c_str());
}

bool OverworldScene::consumeScriptAdvanceRequested() {
    const bool requested = scriptAdvanceRequested_;
    scriptAdvanceRequested_ = false;
    if (!requested) {
        return false;
    }

    if (dialogueOverlay_.isVisible() && !dialogueOverlay_.isFullyRevealed()) {
        dialogueOverlay_.revealAll();
        return false;
    }

    return true;
}
