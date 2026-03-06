#include "engine/map/CompiledMapLoader.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

#include "engine/Map.h"

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
} // namespace

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
