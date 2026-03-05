#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "engine/utils/Vector2D.h"

class GameState;

class OverworldDevConsole {
public:
    struct PlayerPosition {
        float x = 0.0f;
        float y = 0.0f;
        int tileX = 0;
        int tileY = 0;
    };

    struct Dependencies {
        std::function<void(const std::string&)> print;
        std::function<std::vector<std::string>()> listMapIds;
        std::function<std::string()> currentMapId;
        std::function<std::vector<std::string>()> listScriptIds;
        std::function<std::vector<std::string>()> listEncounterTableIds;
        std::function<bool(const std::string&)> runScript;
        std::function<bool(const std::string&)> triggerEncounter;
        std::function<bool()> stopScript;
        std::function<bool(const std::string&, const std::string&)> warpToSpawn;
        std::function<bool(const std::string&, const Vector2D&)> warpToPoint;
        std::function<bool(const Vector2D&)> teleportToPoint;
        std::function<bool()> reloadCurrentMap;
        std::function<std::vector<std::string>()> listSpawnIds;
        std::function<std::optional<PlayerPosition>()> playerPosition;
        std::function<std::string()> partySummary;
        std::function<Vector2D(int, int)> tileToPixel;
        GameState* gameState = nullptr;
    };

    explicit OverworldDevConsole(Dependencies dependencies);

    void execute(const std::string& commandLine) const;

private:
    Dependencies dependencies_;
};
