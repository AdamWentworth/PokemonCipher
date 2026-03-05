#include "game/OverworldScene.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

#include <SDL3/SDL_keyboard.h>

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
    gridMovementSystem_.setInputEnabled(!debugConsoleOpen_ && !scriptInputLocked_ && !startMenuOverlay_.isOpen());
}

void OverworldScene::setDebugConsoleOpen(const bool isOpen) {
    if (debugConsoleOpen_ == isOpen) {
        return;
    }

    debugConsoleOpen_ = isOpen;
    if (debugConsoleOpen_) {
        startMenuOverlay_.close();
        startMenuOverlay_.clearStatusText();
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

void OverworldScene::printConsole(const std::string& message) const {
    std::cout << "[dev] " << message << '\n';
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
