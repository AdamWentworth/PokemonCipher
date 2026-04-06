#pragma once

#include <vector>
#include <SDL3/SDL.h>
#include "ecs/Component.h"
#include "ecs/systems/WaypointSystem.h"

class Map {
public:
    Map() = default;
    ~Map() = default;

    void load(const char *path, SDL_Texture *ts);
    void draw(const Camera &cam);
    // A separate entry point is needed because cover tiles must render in a
    // different pass than terrain or they can never appear in front of sprites.
    void drawCover(const Camera &cam);
    
    SDL_Texture *tileset = nullptr;
    int width{}, height{};
    std::vector<std::vector<int>> tileData;
    // The old loader kept only one tile layer, so TMX cover data needed its
    // own buffer or it would be discarded during load.
    std::vector<std::vector<int>> coverTileData;
    std::vector<Collider> colliders;
    // TMX spawn points let maps choose where a doorway or route exit should
    // place the player after a scene change.
    std::vector<SpawnPoint> spawnPoints;
    // TMX warp rectangles say which map to load next and which spawn point to use there.
    std::vector<WarpPoint> warps;
};
