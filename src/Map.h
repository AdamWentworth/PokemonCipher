#ifndef ASSIGNMENT2_MAP_H
#define ASSIGNMENT2_MAP_H

#include <string>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL.h>

#include "ecs/Component.h"

class Map {
public:
    Map() = default;
    ~Map() = default;

    void load(const char* path, SDL_Texture* ts);
    void draw(const Camera& cam);

    SDL_Texture* tileset = nullptr;
    int width{};
    int height{};
    int tileWidth{16};
    int tileHeight{16};
    int renderScale{2};

    std::vector<std::vector<std::vector<int>>> layers;
    std::vector<Collider> colliders;
    std::unordered_map<std::string, Vector2D> spawnPoints;

    Vector2D getSpawnPoint(const std::string& spawnId, const Vector2D& fallback = Vector2D{}) const;
};

#endif // ASSIGNMENT2_MAP_H
