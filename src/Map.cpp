#include "Map.h"
#include "TextureManager.h"
#include <cmath>
#include <sstream>
#include <string>
#include <tinyxml2.h>

// These values come from the tileset section of the TMX file.
//
// firstGid:
//   The first global tile ID used by this tileset.
//   Tiled does not store tile IDs as 0, 1, 2 relative to the image.
//   It stores global IDs (gids), so we subtract firstGid later
//   to convert a map tile's gid into a tileset-local index.
//
// sourceTileW / sourceTileH:
//   The size of one tile inside the tileset image.
//
// tilesetColumns:
//   How many tiles fit across one row of the tileset image.
//   We use this to convert a tile index into (x, y) coordinates
//   inside the tileset texture
int firstGid = 1;
int sourceTileW = 32;
int sourceTileH = 32;
int tilesetColumns = 1;

namespace {
// Centralizing the rendered tile size keeps map drawing and TMX object scaling
// in sync, so collision fixes do not drift if we adjust the on-screen tile size later.
constexpr float kRenderedTileSize = 32.0f;

// Both terrain and cover layers use the same TMX CSV format, so a shared
// parser keeps the two passes consistent and avoids duplicating load logic.
void parseLayerData(tinyxml2::XMLElement* layer, int width, int height, std::vector<std::vector<int>>& target) {
    target = std::vector(height, std::vector<int>(width, 0));

    if (!layer) return;

    auto* data = layer->FirstChildElement("data");
    if (!data || !data->GetText()) return;

    std::stringstream ss(data->GetText());
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            std::string val;
            if (!std::getline(ss, val, ',')) return;
            target[row][col] = std::stoi(val);
        }
    }
}

// Warp and spawn data live inside <properties>, so this helper keeps that
// lookup in one place instead of repeating the same XML walk for every object.
const char* getObjectPropertyValue(tinyxml2::XMLElement* object, const char* propertyName) {
    auto* properties = object->FirstChildElement("properties");
    if (!properties) return nullptr;

    for (auto* property = properties->FirstChildElement("property");
        property != nullptr;
        property = property->NextSiblingElement("property")) {
        const char* name = property->Attribute("name");
        if (name && std::string(name) == propertyName) {
            return property->Attribute("value");
        }
    }

    return nullptr;
}

// Terrain and cover need identical tile math so they stay perfectly aligned;
// sharing the render path prevents subtle drift between the two passes.
void drawLayer(SDL_Texture* tileset, const Camera& cam, int width, int height, const std::vector<std::vector<int>>& layerData) {
    SDL_FRect src{}, dest{};
    // Use the shared render size here so the drawn map matches the same scale
    // used when converting TMX object-layer coordinates into gameplay space.
    dest.w = dest.h = kRenderedTileSize;

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {

            // gid = the tile ID read from the TMX layer data.
            int gid = layerData[row][col];

            // In Tiled, gid 0 means "empty cell", so skip it.
            if (gid == 0) continue;

            // NEW:
            // Convert the global tile ID into an index inside this tileset.
            //
            // Example:
            //   firstGid = 1
            //   gid = 1  -> tileIndex = 0  (first tile in tileset)
            //   gid = 2  -> tileIndex = 1
            //   gid = 10 -> tileIndex = 9
            int tileIndex = gid - firstGid;

            // If the result is negative, that gid does not belong to this tileset.
            // For a single-tileset map this usually should not happen.
            if (tileIndex < 0) continue;

            // NEW:
            // Compute the source rectangle inside the tileset image.
            //
            // tileIndex % tilesetColumns  -> column in the tileset image
            // tileIndex / tilesetColumns  -> row in the tileset image
            //
            // Then multiply by tile width/height to get pixel coordinates.
            src.x = static_cast<float>((tileIndex % tilesetColumns) * sourceTileW);
            src.y = static_cast<float>((tileIndex / tilesetColumns) * sourceTileH);
            src.w = static_cast<float>(sourceTileW);
            src.h = static_cast<float>(sourceTileH);

            float worldX = static_cast<float>(col) * dest.w;
            float worldY = static_cast<float>(row) * dest.h;

            dest.x = std::round(worldX - cam.view.x);
            dest.y = std::round(worldY - cam.view.y);

            TextureManager::draw(tileset, src, dest);
        }
    }
}
}

void Map::load(const char *path, SDL_Texture *ts) {
    tileset = ts;
    tinyxml2::XMLDocument doc;
    doc.LoadFile(path);

    auto* mapNode = doc.FirstChildElement("map");
    width = mapNode->IntAttribute("width");
    height = mapNode->IntAttribute("height");
    colliders.clear();
    interactions.clear();
    spawnPoints.clear();
    warps.clear();
    // TMX object layers are stored in the map's source pixel size, but this
    // project draws every tile at 32x32. We scale object data during load so
    // collisions and point markers line up with the rendered world.
    const int mapTileW = mapNode->IntAttribute("tilewidth", 16);
    const int mapTileH = mapNode->IntAttribute("tileheight", 16);
    const float objectScaleX = kRenderedTileSize / static_cast<float>(mapTileW);
    const float objectScaleY = kRenderedTileSize / static_cast<float>(mapTileH);

    // NEW:
    // Read metadata from the <tileset> node so we do not need
    // a switch/case for every tile type anymore.
    //
    // Instead of saying:
    //   case 1 = dirt
    //   case 2 = grass
    //   case 4 = water
    //
    // we now calculate the correct source rectangle directly
    // from the gid stored in the map data.
    if (auto* tilesetNode = mapNode->FirstChildElement("tileset")) {
        firstGid = tilesetNode->IntAttribute("firstgid", 1);
        sourceTileW = tilesetNode->IntAttribute("tilewidth", 32);
        sourceTileH = tilesetNode->IntAttribute("tileheight", 32);
        tilesetColumns = tilesetNode->IntAttribute("columns", 1);
    }

    // The original loader only kept the first <layer>, which meant TMX cover
    // tiles were silently ignored. We split them here so both passes survive load.
    bool terrainLoaded = false;
    bool coverLoaded = false;
    for (auto* layer = mapNode->FirstChildElement("layer");
        layer != nullptr;
        layer = layer->NextSiblingElement("layer")) {

        const char* layerName = layer->Attribute("name");
        const bool isCoverLayer = layerName && std::string(layerName).find("Cover") != std::string::npos;

        if (isCoverLayer && !coverLoaded) {
            parseLayerData(layer, width, height, coverTileData);
            coverLoaded = true;
        } else if (!isCoverLayer && !terrainLoaded) {
            parseLayerData(layer, width, height, tileData);
            terrainLoaded = true;
        }
    }

    if (!terrainLoaded) {
        tileData = std::vector(height, std::vector<int>(width, 0));
    }
    if (!coverLoaded) {
        // Keeping an empty cover buffer lets the renderer call drawCover()
        // unconditionally, so maps without a cover layer do not need a special case.
        coverTileData = std::vector(height, std::vector<int>(width, 0));
    }

    // Parse object groups by their "name" value, not by XML order.
    // This keeps loading stable even if groups are reordered or extra groups are added.
    for (auto* objectGroup = mapNode->FirstChildElement("objectgroup");
        objectGroup != nullptr;
        objectGroup = objectGroup->NextSiblingElement("objectgroup")) {
        const char* layerName = objectGroup->Attribute("name");
        if (!layerName) continue;
        const std::string groupName(layerName);
        // Collision is the only object-group gameplay data we still use here,
        // since pickup objects were removed from the map code.
        const bool isCollisionLayer = groupName.find("Collision") != std::string::npos;
        // These point markers tell the scene where to place the player after
        // entering from a door, stair, or route change.
        const bool isSpawnLayer = groupName.find("Player Spawn Points") != std::string::npos;
        // This first dialogue pass uses both a general interaction layer and
        // the existing NPC layer, so Oak can work before a fuller NPC system exists.
        const bool isInteractionLayer =
            groupName.find("Interaction Layer") != std::string::npos ||
            groupName.find("NPC Layer") != std::string::npos;
        // These rectangles mark the spots that should switch the player into
        // another map when they are touched.
        const bool isWarpLayer = groupName.find("Warp Layer") != std::string::npos;

        // Collision layers contribute rectangle colliders.
        if (isCollisionLayer) {
            for (auto* obj = objectGroup->FirstChildElement("object");
                obj != nullptr;
                obj = obj->NextSiblingElement("object")) {

                Collider c;
                // Scale each wall rectangle into the same 32x32 world space as the
                // visible tiles, otherwise the player collides with a half-size map.
                c.rect.x = obj->FloatAttribute("x") * objectScaleX;
                c.rect.y = obj->FloatAttribute("y") * objectScaleY;
                c.rect.w = obj->FloatAttribute("width") * objectScaleX;
                c.rect.h = obj->FloatAttribute("height") * objectScaleY;
                colliders.push_back(c);
            }
        } else if (isSpawnLayer) {
            for (auto* obj = objectGroup->FirstChildElement("object");
                obj != nullptr;
                obj = obj->NextSiblingElement("object")) {

                SpawnPoint spawn;
                const char* spawnName = obj->Attribute("name");
                spawn.id = spawnName ? spawnName : "default";
                spawn.position.x = obj->FloatAttribute("x") * objectScaleX;
                spawn.position.y = obj->FloatAttribute("y") * objectScaleY;
                spawnPoints.push_back(spawn);
            }
        } else if (isInteractionLayer) {
            for (auto* obj = objectGroup->FirstChildElement("object");
                obj != nullptr;
                obj = obj->NextSiblingElement("object")) {

                InteractionPoint interaction;
                const char* interactionId = getObjectPropertyValue(obj, "interaction_id");
                if (!interactionId) {
                    // Script ids are already a common map property in this repo,
                    // so reading them here keeps this first interaction pass reusable.
                    interactionId = getObjectPropertyValue(obj, "script_id");
                }
                if (!interactionId) {
                    interactionId = obj->Attribute("name");
                }
                if (!interactionId) continue;

                interaction.id = interactionId;
                interaction.rect.x = obj->FloatAttribute("x") * objectScaleX;
                interaction.rect.y = obj->FloatAttribute("y") * objectScaleY;
                // Point objects are easier to place in Tiled, so we treat a
                // zero-size object as one interactable tile.
                const float objectWidth = obj->FloatAttribute("width");
                const float objectHeight = obj->FloatAttribute("height");
                interaction.rect.w = objectWidth > 0.0f ? objectWidth * objectScaleX : kRenderedTileSize;
                interaction.rect.h = objectHeight > 0.0f ? objectHeight * objectScaleY : kRenderedTileSize;
                interactions.push_back(interaction);
            }
        } else if (isWarpLayer) {
            for (auto* obj = objectGroup->FirstChildElement("object");
                obj != nullptr;
                obj = obj->NextSiblingElement("object")) {

                WarpPoint warp;
                warp.rect.x = obj->FloatAttribute("x") * objectScaleX;
                warp.rect.y = obj->FloatAttribute("y") * objectScaleY;
                warp.rect.w = obj->FloatAttribute("width") * objectScaleX;
                warp.rect.h = obj->FloatAttribute("height") * objectScaleY;

                if (const char* targetMap = getObjectPropertyValue(obj, "target_map")) {
                    warp.targetMap = targetMap;
                }
                if (const char* targetSpawnId = getObjectPropertyValue(obj, "target_spawn_id")) {
                    warp.targetSpawnId = targetSpawnId;
                }
                if (const char* requiredDirection = getObjectPropertyValue(obj, "required_direction")) {
                    // Some door and stair warps should only fire when the player
                    // steps into that tile from the matching side.
                    warp.requiredDirection = requiredDirection;
                }

                const char* targetSpawnX = getObjectPropertyValue(obj, "target_spawn_x");
                const char* targetSpawnY = getObjectPropertyValue(obj, "target_spawn_y");
                if (targetSpawnX && targetSpawnY) {
                    // Some map exits land on a fixed spot instead of a named
                    // spawn point, so this simple version supports both.
                    warp.targetPosition.x = std::stof(targetSpawnX) * objectScaleX;
                    warp.targetPosition.y = std::stof(targetSpawnY) * objectScaleY;
                    warp.hasTargetPosition = true;
                }

                if (!warp.targetMap.empty()) {
                    warps.push_back(warp);
                }
            }
        }
    }
}

void Map::draw(const Camera &cam) {
    // Terrain stays in the normal map pass so entities can still appear on top of it.
    drawLayer(tileset, cam, width, height, tileData);
}

void Map::drawCover(const Camera &cam) {
    // Cover tiles need their own pass because roofs/tree tops only work if they
    // render after sprites, not alongside the base terrain.
    drawLayer(tileset, cam, width, height, coverTileData);
}
