#pragma once

#include <SDL3/SDL.h>

#include <string>
#include <vector>

#include "utils/Vector2D.h"

struct WarpPoint {
    SDL_FRect area{};
    std::string targetMap;
    Vector2D targetSpawn{};
};

struct EncounterZone {
    SDL_FRect area{};
    std::string tableId;
};

class Map {
public:
    bool load(const char* path);

    int width = 0;
    int height = 0;
    int tileWidth = 32;
    int tileHeight = 32;

    std::vector<std::vector<int>> groundTileData;
    std::vector<std::vector<int>> coverTileData;
    std::vector<SDL_FRect> blockingRects;
    std::vector<WarpPoint> warpPoints;
    std::vector<EncounterZone> encounterZones;
    Vector2D playerSpawn{64.0f, 64.0f};
};
