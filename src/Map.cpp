#include "Map.h"
#include "MapLoader.h"
#include "TextureManager.h"
#include <cmath>

namespace {
// Centralizing the rendered tile size keeps map drawing and TMX object scaling
// in sync, so collision fixes do not drift if we adjust the on-screen tile size later.
constexpr float kRenderedTileSize = 32.0f;

// Terrain and cover need identical tile math so they stay perfectly aligned;
// sharing the render path prevents subtle drift between the two passes.
void drawLayer(const Map& map, const Camera& cam, const std::vector<std::vector<int>>& layerData) {
    SDL_FRect src{}, dest{};
    // Use the shared render size here so the drawn map matches the same scale
    // used when converting TMX object-layer coordinates into gameplay space.
    dest.w = dest.h = kRenderedTileSize;

    for (int row = 0; row < map.height; row++) {
        for (int col = 0; col < map.width; col++) {

            // gid = the tile ID read from the TMX layer data.
            int gid = layerData[row][col];

            // In Tiled, gid 0 means "empty cell", so skip it.
            if (gid == 0) continue;

            // Convert the global tile ID into an index inside this tileset.
            //
            // Example:
            //   firstGid = 1
            //   gid = 1  -> tileIndex = 0  (first tile in tileset)
            //   gid = 2  -> tileIndex = 1
            //   gid = 10 -> tileIndex = 9
            int tileIndex = gid - map.firstGid;

            // If the result is negative, that gid does not belong to this tileset.
            // For a single-tileset map this usually should not happen.
            if (tileIndex < 0) continue;

            // Compute the source rectangle inside the tileset image.
            //
            // tileIndex % tilesetColumns  -> column in the tileset image
            // tileIndex / tilesetColumns  -> row in the tileset image
            //
            // Then multiply by tile width/height to get pixel coordinates.
            src.x = static_cast<float>((tileIndex % map.tilesetColumns) * map.sourceTileW);
            src.y = static_cast<float>((tileIndex / map.tilesetColumns) * map.sourceTileH);
            src.w = static_cast<float>(map.sourceTileW);
            src.h = static_cast<float>(map.sourceTileH);

            float worldX = static_cast<float>(col) * dest.w;
            float worldY = static_cast<float>(row) * dest.h;

            dest.x = std::round(worldX - cam.view.x);
            dest.y = std::round(worldY - cam.view.y);

            TextureManager::draw(map.tileset, src, dest);
        }
    }
}
}

void Map::load(const char *path, SDL_Texture *ts) {
    // TMX parsing moved into its own loader so this file can stay focused on
    // drawing an already-loaded map instead of knowing XML details too.
    MapLoader::loadInto(*this, path, ts);
}

void Map::draw(const Camera &cam) {
    // Terrain stays in the normal map pass so entities can still appear on top of it.
    drawLayer(*this, cam, tileData);
}

void Map::drawCover(const Camera &cam) {
    // Cover tiles need their own pass because roofs/tree tops only work if they
    // render after sprites, not alongside the base terrain.
    drawLayer(*this, cam, coverTileData);
}
