#pragma once

#include <SDL3/SDL.h>

#include <vector>

#include "Map.h"
#include "TextureManager.h"
#include "ecs/Component.h"

class TilemapRenderer {
public:
    TilemapRenderer(TextureManager& textureManager, const char* baseTilesetPath, const char* coverTilesetPath = nullptr);

    void renderGround(const Map& map, const Camera& camera);
    void renderCover(const Map& map, const Camera& camera);

private:
    void renderLayer(
        const Map& map,
        const Camera& camera,
        const std::vector<std::vector<int>>& layerData,
        SDL_Texture* texture
    );
    int getTilesetColumns(SDL_Texture* texture, const Map& map) const;

    TextureManager& textureManager_;
    SDL_Texture* baseTileset_ = nullptr;
    SDL_Texture* coverTileset_ = nullptr;
};
