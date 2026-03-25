#include "Map.h"
#include "TextureManager.h"
#include <sstream>    // std::stringstream for parsing CSV layer data
#include <string>     // std::string
#include <vector>     // std::vector
#include <tinyxml2.h> // XML parser used to read the .tmx map file

namespace {
    // All parsed tile layers from the map.
    //
    // Structure:
    //   g_layers[layerIndex][row][col] = gid
    //
    // Each gid is the "global tile ID" from Tiled.
    // A value of 0 means "no tile".
    std::vector<std::vector<std::vector<int>>> g_layers;

    // firstgid from the <tileset> tag.
    // Tiled stores tile IDs globally, not starting at 0 for each tileset.
    // We subtract this value later to turn a gid into a tileset-local index.
    int g_firstGid = 1;

    // Original tile dimensions inside the tileset image.
    // These are read from the <tileset> node in the TMX file.
    int g_tileWidth = 16;
    int g_tileHeight = 16;

    // Number of columns in the tileset image.
    // Needed to convert a tile index into (x, y) source coordinates.
    int g_columns = 1;
}

void Map::load(const char *path, SDL_Texture *ts) {
    // Store the tileset texture that will be used later when drawing tiles.
    // This texture is expected to match the tileset referenced by the map file.
    tileset = ts;

    // Load and parse the XML map file (.tmx).
    tinyxml2::XMLDocument doc;
    doc.LoadFile(path);

    // Get the root <map> element.
    auto* mapNode = doc.FirstChildElement("map");

    // Read the map dimensions in tiles, not pixels.
    // Example: width=100, height=50 means 100 columns and 50 rows of tiles.
    width = mapNode->IntAttribute("width");
    height = mapNode->IntAttribute("height");

    // Clear any previous map-specific data before loading a new map.
    colliders.clear();
    itemSpawnPoints.clear();
    g_layers.clear();

    // Read the first <tileset> entry from the map.
    // This gives us the information needed to interpret tile IDs and
    // cut the correct tile rectangles from the tileset texture.
    auto* tilesetNode = mapNode->FirstChildElement("tileset");
    if (tilesetNode) {
        // firstgid = first global tile ID belonging to this tileset
        g_firstGid = tilesetNode->IntAttribute("firstgid", 1);

        // Dimensions of a single tile inside the source tileset image
        g_tileWidth = tilesetNode->IntAttribute("tilewidth", 16);
        g_tileHeight = tilesetNode->IntAttribute("tileheight", 16);

        // Number of columns in the tileset image
        g_columns = tilesetNode->IntAttribute("columns", 1);
    }

    // Iterate through all <layer> elements.
    // These are tile layers, usually exported by Tiled as CSV.
    for (auto* layer = mapNode->FirstChildElement("layer");
         layer != nullptr;
         layer = layer->NextSiblingElement("layer")) {

        // Inside each <layer>, the <data> node contains the tile IDs.
        auto* data = layer->FirstChildElement("data");

        // Skip malformed or empty layers.
        if (!data || !data->GetText()) continue;

        // Read the CSV text content from the layer.
        // Example content:
        //   1,2,0,0,5,...
        std::string csv = data->GetText();
        std::stringstream ss(csv);

        // Create a 2D grid [height][width] for this layer and initialize to 0.
        // Each entry stores the gid for one tile cell.
        std::vector<std::vector<int>> layerData(height, std::vector<int>(width, 0));

        // Parse CSV values row by row, column by column.
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                std::string val;

                // Read up to the next comma.
                // If parsing ends early, stop filling.
                if (!std::getline(ss, val, ',')) break;

                // Convert the text value into an integer gid.
                layerData[i][j] = std::stoi(val);
            }
        }

        // Store this finished layer in the global layer list.
        g_layers.push_back(layerData);
    }

    // Iterate through all object layers (<objectgroup>).
    // These are typically used for collision boxes, spawn points, triggers, etc.
    for (auto* objectGroup = mapNode->FirstChildElement("objectgroup");
         objectGroup != nullptr;
         objectGroup = objectGroup->NextSiblingElement("objectgroup")) {

        // Read the object group name, e.g. "Collision", "Items", etc.
        const char* layerName = objectGroup->Attribute("name");
        if (!layerName) continue;

        std::string groupName(layerName);

        // Decide what this object layer represents based on its name.
        // This is a simple substring check, not an exact match.
        bool isCollisionLayer = groupName.find("Collision") != std::string::npos;
        bool isItemLayer = groupName.find("Item") != std::string::npos;

        // Collision object layer:
        // every object is converted into a rectangular collider.
        if (isCollisionLayer) {
            for (auto* obj = objectGroup->FirstChildElement("object");
                 obj != nullptr;
                 obj = obj->NextSiblingElement("object")) {

                Collider c;

                // Read rectangle position and size from the object.
                // Tiled stores these in map/world pixel coordinates.
                c.rect.x = obj->FloatAttribute("x");
                c.rect.y = obj->FloatAttribute("y");
                c.rect.w = obj->FloatAttribute("width");
                c.rect.h = obj->FloatAttribute("height");

                // Save collider for gameplay collision checks.
                colliders.push_back(c);
            }
        }

        // Item object layer:
        // only point objects are used, and each point becomes an item spawn location.
        if (isItemLayer) {
            for (auto* obj = objectGroup->FirstChildElement("object");
                 obj != nullptr;
                 obj = obj->NextSiblingElement("object")) {

                // Ignore anything that is not explicitly a point object.
                if (!obj->FirstChildElement("point")) continue;

                // Store the point position as an item spawn point.
                itemSpawnPoints.push_back(
                    Vector2D(obj->FloatAttribute("x"), obj->FloatAttribute("y"))
                );
            }
        }
    }
}

void Map::draw(const Camera &cam) {
    // Source rectangle inside the tileset texture.
    // This selects which tile image to copy.
    SDL_FRect src{};

    // Destination rectangle on screen.
    // This is where the tile will be drawn after camera offset is applied.
    SDL_FRect dest{};

    // Tiles are rendered as 32x32 on screen.
    // Note: this is independent from the source tile size in the tileset.
    // For example, a 16x16 tile can be drawn scaled up to 32x32.
    dest.w = 32;
    dest.h = 32;

    // Draw every tile in every loaded layer, in order.
    // Earlier layers are drawn first; later layers appear on top.
    for (const auto& layer : g_layers) {
        for (int row = 0; row < height; row++) {
            for (int col = 0; col < width; col++) {

                // gid = global tile ID at this cell.
                int gid = layer[row][col];

                // gid == 0 means the cell is empty, so skip it.
                if (gid == 0) continue;

                // Convert global tile ID into local tileset index.
                int tileIndex = gid - g_firstGid;

                // Negative means the gid does not belong to this tileset.
                // Skip invalid/unsupported tiles.
                if (tileIndex < 0) continue;

                // Convert tile index into source texture coordinates.
                //
                // Example:
                //   tileIndex 0 -> first tile in top-left
                //   x = (index % columns) * tileWidth
                //   y = (index / columns) * tileHeight
                src.x = static_cast<float>((tileIndex % g_columns) * g_tileWidth);
                src.y = static_cast<float>((tileIndex / g_columns) * g_tileHeight);
                src.w = static_cast<float>(g_tileWidth);
                src.h = static_cast<float>(g_tileHeight);

                // Convert tile grid position into screen position.
                //
                // col * dest.w gives the tile's world X position in pixels
                // row * dest.h gives the tile's world Y position in pixels
                //
                // Then subtract the camera position so the map scrolls
                // relative to what the camera is looking at.
                //
                // std::round helps avoid subpixel rendering artifacts.
                dest.x = std::round(col * dest.w - cam.view.x);
                dest.y = std::round(row * dest.h - cam.view.y);

                // Draw the selected tile region from the tileset texture
                // into the destination rectangle on screen.
                TextureManager::draw(tileset, src, dest);
            }
        }
    }
}