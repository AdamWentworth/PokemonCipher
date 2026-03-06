#pragma once

#include <SDL3/SDL.h>

#include <unordered_map>
#include <string>
#include <vector>

#include "utils/Vector2D.h"

struct WarpPoint {
    SDL_FRect area{};
    std::string targetMap;
    std::string targetSpawnId;
    std::string requiredDirection;
    bool hasTargetSpawn = false;
    Vector2D targetSpawn{};
};

struct EncounterZone {
    SDL_FRect area{};
    std::string tableId;
};

struct NpcSpawn {
    SDL_FRect area{};
    std::string id;
    std::string scriptId;
    std::string speaker;
    std::string dialogue;
    std::string facing;
    std::string spritePath;
    std::string animationPath;
};

class Map {
public:
    bool load(const char* path);
    Vector2D getSpawnPoint(const std::string& spawnId) const;

    int width = 0;
    int height = 0;
    int tileWidth = 32;
    int tileHeight = 32;

    std::vector<std::vector<int>> groundTileData;
    std::vector<std::vector<int>> coverTileData;
    std::vector<SDL_FRect> blockingRects;
    std::vector<WarpPoint> warpPoints;
    std::vector<EncounterZone> encounterZones;
    std::vector<NpcSpawn> npcSpawns;
    std::unordered_map<std::string, Vector2D> spawnPoints;
    Vector2D playerSpawn{64.0f, 64.0f};
};
