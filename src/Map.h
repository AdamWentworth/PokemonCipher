#ifndef ASSIGNMENT2_MAP_H
#define ASSIGNMENT2_MAP_H

#include <vector>
#include <SDL3/SDL.h>
#include "ecs/Component.h"

class Map {
public:
    Map() = default;
    ~Map() = default;

    void load(const char *path, SDL_Texture *ts);
    void draw(const Camera &cam);
    
    SDL_Texture *tileset = nullptr;
    int width{}, height{};
    std::vector<std::vector<int>> tileData;
    std::vector<Collider> colliders;
    std::vector<Vector2D> itemSpawnPoints;
};

#endif //ASSIGNMENT2_MAP_H
