#include "Map.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>

#include <tinyxml2.h>

namespace {
constexpr std::uint32_t kCompiledMapVersion = 1;

template <typename T>
bool readBinary(std::ifstream& stream, T& outValue) {
    stream.read(reinterpret_cast<char*>(&outValue), sizeof(T));
    return static_cast<bool>(stream);
}

bool readBinaryString(std::ifstream& stream, std::string& outValue) {
    std::uint32_t length = 0;
    if (!readBinary(stream, length)) {
        return false;
    }

    outValue.clear();
    outValue.resize(length);
    if (length == 0) {
        return true;
    }

    stream.read(outValue.data(), static_cast<std::streamsize>(length));
    return static_cast<bool>(stream);
}

bool readBinaryRect(std::ifstream& stream, SDL_FRect& outRect) {
    return readBinary(stream, outRect.x) &&
           readBinary(stream, outRect.y) &&
           readBinary(stream, outRect.w) &&
           readBinary(stream, outRect.h);
}

bool readBinaryVec2(std::ifstream& stream, Vector2D& outPoint) {
    return readBinary(stream, outPoint.x) && readBinary(stream, outPoint.y);
}

bool loadCompiledMap(Map& map, const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return false;
    }

    char magic[4]{};
    stream.read(magic, sizeof(magic));
    if (!stream || magic[0] != 'P' || magic[1] != 'C' || magic[2] != 'M' || magic[3] != 'P') {
        return false;
    }

    std::uint32_t version = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t tileWidth = 0;
    std::uint32_t tileHeight = 0;
    std::uint32_t groundCount = 0;
    std::uint32_t coverCount = 0;
    std::uint32_t blockingCount = 0;
    std::uint32_t warpCount = 0;
    std::uint32_t encounterCount = 0;
    std::uint32_t npcCount = 0;
    std::uint32_t spawnCount = 0;
    float playerSpawnX = 0.0f;
    float playerSpawnY = 0.0f;

    if (!readBinary(stream, version) || version != kCompiledMapVersion ||
        !readBinary(stream, width) || !readBinary(stream, height) ||
        !readBinary(stream, tileWidth) || !readBinary(stream, tileHeight) ||
        !readBinary(stream, groundCount) || !readBinary(stream, coverCount) ||
        !readBinary(stream, blockingCount) || !readBinary(stream, warpCount) ||
        !readBinary(stream, encounterCount) || !readBinary(stream, npcCount) ||
        !readBinary(stream, spawnCount) ||
        !readBinary(stream, playerSpawnX) || !readBinary(stream, playerSpawnY)) {
        return false;
    }

    if (width == 0 || height == 0 || tileWidth == 0 || tileHeight == 0) {
        return false;
    }

    const std::uint64_t tileCount = static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height);
    if (groundCount != tileCount || coverCount != tileCount) {
        return false;
    }

    map.width = static_cast<int>(width);
    map.height = static_cast<int>(height);
    map.tileWidth = static_cast<int>(tileWidth);
    map.tileHeight = static_cast<int>(tileHeight);
    map.groundTileData.assign(map.height, std::vector<int>(map.width, 0));
    map.coverTileData.assign(map.height, std::vector<int>(map.width, 0));
    map.blockingRects.clear();
    map.warpPoints.clear();
    map.encounterZones.clear();
    map.npcSpawns.clear();
    map.spawnPoints.clear();
    map.playerSpawn = Vector2D(playerSpawnX, playerSpawnY);

    for (std::uint32_t i = 0; i < groundCount; ++i) {
        std::int32_t value = 0;
        if (!readBinary(stream, value)) {
            return false;
        }
        const int row = static_cast<int>(i / width);
        const int col = static_cast<int>(i % width);
        map.groundTileData[row][col] = value;
    }

    for (std::uint32_t i = 0; i < coverCount; ++i) {
        std::int32_t value = 0;
        if (!readBinary(stream, value)) {
            return false;
        }
        const int row = static_cast<int>(i / width);
        const int col = static_cast<int>(i % width);
        map.coverTileData[row][col] = value;
    }

    map.blockingRects.reserve(blockingCount);
    for (std::uint32_t i = 0; i < blockingCount; ++i) {
        SDL_FRect rect{};
        if (!readBinaryRect(stream, rect)) {
            return false;
        }
        map.blockingRects.push_back(rect);
    }

    map.warpPoints.reserve(warpCount);
    for (std::uint32_t i = 0; i < warpCount; ++i) {
        WarpPoint warp{};
        std::uint8_t hasTargetSpawn = 0;
        if (!readBinaryRect(stream, warp.area) ||
            !readBinaryString(stream, warp.targetMap) ||
            !readBinaryString(stream, warp.targetSpawnId) ||
            !readBinaryString(stream, warp.requiredDirection) ||
            !readBinary(stream, hasTargetSpawn)) {
            return false;
        }

        warp.hasTargetSpawn = hasTargetSpawn != 0;
        if (warp.hasTargetSpawn && !readBinaryVec2(stream, warp.targetSpawn)) {
            return false;
        }

        std::transform(
            warp.requiredDirection.begin(),
            warp.requiredDirection.end(),
            warp.requiredDirection.begin(),
            [](const unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            }
        );

        map.warpPoints.push_back(std::move(warp));
    }

    map.encounterZones.reserve(encounterCount);
    for (std::uint32_t i = 0; i < encounterCount; ++i) {
        EncounterZone zone{};
        if (!readBinaryRect(stream, zone.area) || !readBinaryString(stream, zone.tableId)) {
            return false;
        }
        map.encounterZones.push_back(std::move(zone));
    }

    map.npcSpawns.reserve(npcCount);
    for (std::uint32_t i = 0; i < npcCount; ++i) {
        NpcSpawn npc{};
        if (!readBinaryRect(stream, npc.area) ||
            !readBinaryString(stream, npc.id) ||
            !readBinaryString(stream, npc.scriptId) ||
            !readBinaryString(stream, npc.speaker) ||
            !readBinaryString(stream, npc.dialogue) ||
            !readBinaryString(stream, npc.facing) ||
            !readBinaryString(stream, npc.spritePath) ||
            !readBinaryString(stream, npc.animationPath)) {
            return false;
        }
        map.npcSpawns.push_back(std::move(npc));
    }

    bool foundDefaultSpawn = false;
    for (std::uint32_t i = 0; i < spawnCount; ++i) {
        std::string spawnId;
        Vector2D point{};
        if (!readBinaryString(stream, spawnId) || !readBinaryVec2(stream, point)) {
            return false;
        }

        std::transform(spawnId.begin(), spawnId.end(), spawnId.begin(), [](const unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });

        map.spawnPoints[spawnId] = point;
        if (!foundDefaultSpawn && spawnId == "default") {
            map.playerSpawn = point;
            foundDefaultSpawn = true;
        }
    }

    return static_cast<bool>(stream);
}

bool tryLoadCompiledMap(Map& map, const char* sourcePath) {
    if (!sourcePath || sourcePath[0] == '\0') {
        return false;
    }

    const std::filesystem::path inputPath(sourcePath);
    if (inputPath.extension() == ".pcmap") {
        return loadCompiledMap(map, inputPath);
    }

    const std::filesystem::path sidecarPath = inputPath.string() + ".pcmap";
    if (!std::filesystem::exists(sidecarPath)) {
        return false;
    }

    return loadCompiledMap(map, sidecarPath);
}
} // namespace

bool Map::load(const char* path) {
    if (!path || path[0] == '\0') {
        return false;
    }

    if (tryLoadCompiledMap(*this, path)) {
        return true;
    }

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
    npcSpawns.clear();
    spawnPoints.clear();
    bool foundSpawn = false;

    for (auto* objectGroup = mapNode->FirstChildElement("objectgroup");
         objectGroup != nullptr;
         objectGroup = objectGroup->NextSiblingElement("objectgroup")) {

        const char* layerName = objectGroup->Attribute("name");
        if (!layerName) {
            continue;
        }

        std::string groupName = layerName;
        std::transform(groupName.begin(), groupName.end(), groupName.begin(), [](const unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });

        const bool isCollisionLayer = groupName.find("collision") != std::string::npos;
        const bool isSpawnLayer = groupName.find("spawn") != std::string::npos || groupName.find("player") != std::string::npos;
        const bool isWarpLayer = groupName.find("warp") != std::string::npos;
        const bool isEncounterLayer =
            groupName.find("encounter") != std::string::npos ||
            groupName.find("grass") != std::string::npos;
        const bool isNpcLayer =
            groupName.find("npc") != std::string::npos ||
            groupName.find("character") != std::string::npos ||
            groupName.find("object event") != std::string::npos;

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

            if (isSpawnLayer) {
                const float spawnX = object->FloatAttribute("x");
                const float spawnY = object->FloatAttribute("y");

                std::string spawnId = "default";
                if (const char* name = object->Attribute("name"); name && name[0] != '\0') {
                    spawnId = name;
                } else {
                    spawnId = std::to_string(object->IntAttribute("id", 0));
                }

                std::transform(spawnId.begin(), spawnId.end(), spawnId.begin(), [](const unsigned char ch) {
                    return static_cast<char>(std::tolower(ch));
                });

                spawnPoints[spawnId] = Vector2D(spawnX, spawnY);
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

                warp.targetSpawnId = getStringProperty("target_spawn_id");
                if (warp.targetSpawnId.empty()) {
                    warp.targetSpawnId = getStringProperty("targetSpawnId");
                }

                warp.requiredDirection = getStringProperty("required_direction");
                if (warp.requiredDirection.empty()) {
                    warp.requiredDirection = getStringProperty("requiredDirection");
                }
                std::transform(
                    warp.requiredDirection.begin(),
                    warp.requiredDirection.end(),
                    warp.requiredDirection.begin(),
                    [](const unsigned char ch) {
                        return static_cast<char>(std::tolower(ch));
                    }
                );

                const std::string spawnX = getStringProperty("target_spawn_x");
                const std::string spawnY = getStringProperty("target_spawn_y");
                if (!spawnX.empty() && !spawnY.empty()) {
                    warp.targetSpawn.x = std::stof(spawnX);
                    warp.targetSpawn.y = std::stof(spawnY);
                    warp.hasTargetSpawn = true;
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

            if (isNpcLayer) {
                NpcSpawn npc{};
                npc.area = getRectFromObject();
                npc.scriptId = getStringProperty("script_id");
                if (npc.scriptId.empty()) {
                    npc.scriptId = getStringProperty("script");
                }
                npc.speaker = getStringProperty("speaker");
                npc.dialogue = getStringProperty("dialogue");
                if (npc.dialogue.empty()) {
                    npc.dialogue = getStringProperty("text");
                }
                npc.facing = getStringProperty("facing");
                npc.spritePath = getStringProperty("sprite");
                npc.animationPath = getStringProperty("animation");

                if (const char* name = object->Attribute("name"); name && name[0] != '\0') {
                    npc.id = name;
                } else {
                    npc.id = std::to_string(object->IntAttribute("id", 0));
                }

                npcSpawns.push_back(std::move(npc));
            }
        }
    }

    return true;
}

Vector2D Map::getSpawnPoint(const std::string& spawnId) const {
    std::string key = spawnId;
    std::transform(key.begin(), key.end(), key.begin(), [](const unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    const auto it = spawnPoints.find(key);
    if (it != spawnPoints.end()) {
        return it->second;
    }

    return playerSpawn;
}
