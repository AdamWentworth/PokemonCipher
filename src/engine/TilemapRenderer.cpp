#include "TilemapRenderer.h"

#include <algorithm>
#include <cmath>

TilemapRenderer::TilemapRenderer(
    TextureManager& textureManager,
    const char* baseTilesetPath,
    const char* coverTilesetPath
)
    : textureManager_(textureManager) {
    setTilesets(baseTilesetPath, coverTilesetPath);
}

void TilemapRenderer::setTilesets(const char* baseTilesetPath, const char* coverTilesetPath) {
    baseTileset_ = baseTilesetPath ? textureManager_.load(baseTilesetPath) : nullptr;
    coverTileset_ = coverTilesetPath ? textureManager_.load(coverTilesetPath) : nullptr;
}

int TilemapRenderer::getTilesetColumns(SDL_Texture* texture, const Map& map) const {
    if (!texture || map.tileWidth <= 0) {
        return 1;
    }

    float textureWidth = 0.0f;
    float textureHeight = 0.0f;
    if (!SDL_GetTextureSize(texture, &textureWidth, &textureHeight)) {
        return 1;
    }

    return std::max(1, static_cast<int>(textureWidth) / map.tileWidth);
}

void TilemapRenderer::renderLayer(
    const Map& map,
    const Camera& camera,
    const std::vector<std::vector<int>>& layerData,
    SDL_Texture* texture
) {
    if (!texture) {
        return;
    }

    const int tilesetColumns = getTilesetColumns(texture, map);

    SDL_FRect src{};
    SDL_FRect dst{};

    src.w = static_cast<float>(map.tileWidth);
    src.h = static_cast<float>(map.tileHeight);
    dst.w = src.w;
    dst.h = src.h;

    for (int row = 0; row < map.height; ++row) {
        for (int col = 0; col < map.width; ++col) {
            const int gid = layerData[row][col];
            if (gid <= 0) {
                continue;
            }

            const int tileIndex = gid - 1;
            const int srcColumn = tileIndex % tilesetColumns;
            const int srcRow = tileIndex / tilesetColumns;

            src.x = static_cast<float>(srcColumn * map.tileWidth);
            src.y = static_cast<float>(srcRow * map.tileHeight);

            const float worldX = static_cast<float>(col * map.tileWidth);
            const float worldY = static_cast<float>(row * map.tileHeight);

            dst.x = std::round(worldX - camera.view.x);
            dst.y = std::round(worldY - camera.view.y);

            textureManager_.draw(texture, src, dst);
        }
    }
}

void TilemapRenderer::renderGround(const Map& map, const Camera& camera) {
    renderLayer(map, camera, map.groundTileData, baseTileset_);
}

void TilemapRenderer::renderCover(const Map& map, const Camera& camera) {
    if (!coverTileset_) {
        return;
    }

    renderLayer(map, camera, map.coverTileData, coverTileset_);
}
