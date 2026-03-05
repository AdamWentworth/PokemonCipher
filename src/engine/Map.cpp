#include "Map.h"

#include <sstream>
#include <string>

#include <tinyxml2.h>

bool Map::load(const char* path) {
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(path) != tinyxml2::XML_SUCCESS) {
        return false;
    }

    auto* mapNode = doc.FirstChildElement("map");
    if (!mapNode) {
        return false;
    }

    width = mapNode->IntAttribute("width");
    height = mapNode->IntAttribute("height");
    tileWidth = mapNode->IntAttribute("tilewidth", 32);
    tileHeight = mapNode->IntAttribute("tileheight", 32);

    groundTileData.assign(height, std::vector<int>(width, 0));
    coverTileData.assign(height, std::vector<int>(width, 0));

    auto parseCsvLayer = [this](tinyxml2::XMLElement* layerNode, std::vector<std::vector<int>>& target) {
        if (!layerNode) {
            return;
        }

        auto* data = layerNode->FirstChildElement("data");
        if (!data) {
            return;
        }

        const char* csvText = data->GetText();
        if (!csvText) {
            return;
        }

        std::stringstream stream(csvText);
        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {
                std::string value;
                if (!std::getline(stream, value, ',')) {
                    return;
                }

                try {
                    target[row][col] = std::stoi(value);
                } catch (...) {
                    target[row][col] = 0;
                }
            }
        }
    };

    bool parsedGround = false;
    for (auto* layer = mapNode->FirstChildElement("layer");
         layer != nullptr;
         layer = layer->NextSiblingElement("layer")) {
        const char* layerNameAttr = layer->Attribute("name");
        const std::string layerName = layerNameAttr ? layerNameAttr : "";

        const bool isCoverLayer =
            layerName.find("Cover") != std::string::npos ||
            layerName.find("Overlay") != std::string::npos ||
            layerName.find("Top") != std::string::npos;

        const bool isGroundLayer =
            layerName.find("Terrain") != std::string::npos ||
            layerName.find("Ground") != std::string::npos ||
            layerName.find("Base") != std::string::npos;

        if (isCoverLayer) {
            parseCsvLayer(layer, coverTileData);
            continue;
        }

        if (isGroundLayer || !parsedGround) {
            parseCsvLayer(layer, groundTileData);
            parsedGround = true;
        }
    }

    blockingRects.clear();
    warpPoints.clear();
    encounterZones.clear();
    bool foundSpawn = false;

    for (auto* objectGroup = mapNode->FirstChildElement("objectgroup");
         objectGroup != nullptr;
         objectGroup = objectGroup->NextSiblingElement("objectgroup")) {

        const char* layerName = objectGroup->Attribute("name");
        if (!layerName) {
            continue;
        }

        const std::string groupName(layerName);
        const bool isCollisionLayer = groupName.find("Collision") != std::string::npos;
        const bool isSpawnLayer = groupName.find("Spawn") != std::string::npos || groupName.find("Player") != std::string::npos;
        const bool isWarpLayer = groupName.find("Warp") != std::string::npos;
        const bool isEncounterLayer =
            groupName.find("Encounter") != std::string::npos ||
            groupName.find("Grass") != std::string::npos;

        for (auto* object = objectGroup->FirstChildElement("object");
             object != nullptr;
             object = object->NextSiblingElement("object")) {

            auto getStringProperty = [object](const char* propertyName) -> std::string {
                const auto* properties = object->FirstChildElement("properties");
                if (!properties) {
                    return "";
                }

                for (const auto* property = properties->FirstChildElement("property");
                     property != nullptr;
                     property = property->NextSiblingElement("property")) {
                    const char* name = property->Attribute("name");
                    if (!name || std::string(name) != propertyName) {
                        continue;
                    }

                    if (const char* value = property->Attribute("value")) {
                        return value;
                    }

                    if (property->GetText()) {
                        return property->GetText();
                    }
                }

                return "";
            };

            auto getRectFromObject = [this, object]() {
                SDL_FRect rect{};
                rect.x = object->FloatAttribute("x");
                rect.y = object->FloatAttribute("y");
                rect.w = object->FloatAttribute("width");
                rect.h = object->FloatAttribute("height");

                if (rect.w <= 0.0f) {
                    rect.w = static_cast<float>(tileWidth);
                }
                if (rect.h <= 0.0f) {
                    rect.h = static_cast<float>(tileHeight);
                }

                return rect;
            };

            if (isCollisionLayer) {
                blockingRects.push_back(getRectFromObject());
            }

            if (isSpawnLayer && !foundSpawn) {
                playerSpawn = Vector2D(object->FloatAttribute("x"), object->FloatAttribute("y"));
                foundSpawn = true;
            }

            if (isWarpLayer) {
                WarpPoint warp{};
                warp.area = getRectFromObject();

                warp.targetMap = getStringProperty("target_map");
                if (warp.targetMap.empty()) {
                    warp.targetMap = getStringProperty("targetMap");
                }

                const char* fallbackTarget = object->Attribute("name");
                if (warp.targetMap.empty() && fallbackTarget) {
                    warp.targetMap = fallbackTarget;
                }

                const std::string spawnX = getStringProperty("target_spawn_x");
                const std::string spawnY = getStringProperty("target_spawn_y");
                if (!spawnX.empty() && !spawnY.empty()) {
                    warp.targetSpawn.x = std::stof(spawnX);
                    warp.targetSpawn.y = std::stof(spawnY);
                }

                warpPoints.push_back(warp);
            }

            if (isEncounterLayer) {
                EncounterZone zone{};
                zone.area = getRectFromObject();
                zone.tableId = getStringProperty("encounter_table");
                if (zone.tableId.empty()) {
                    zone.tableId = getStringProperty("table");
                }
                encounterZones.push_back(zone);
            }
        }
    }

    return true;
}
