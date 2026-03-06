#include "Map.h"

#include <algorithm>
#include <cctype>
#include <string>

#include "engine/map/CompiledMapLoader.h"
#include "engine/map/TmxMapLoader.h"

bool Map::load(const char* path) {
    if (!path || path[0] == '\0') {
        return false;
    }

    if (tryLoadCompiledMap(*this, path)) {
        return true;
    }

    return loadMapFromTmx(*this, path);
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
