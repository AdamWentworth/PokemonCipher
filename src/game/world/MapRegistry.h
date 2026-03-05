#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>
#include <vector>

struct MapDefinition {
    std::string id;
    std::string mapPath;
    std::string baseTilesetPath;
    std::string coverTilesetPath;
};

class MapRegistry {
public:
    void addMap(const MapDefinition& map) {
        const std::string key = normalize(map.id);
        maps_[key] = map;
        aliases_[key] = key;
    }

    void addAlias(const std::string& alias, const std::string& mapId) {
        const std::string aliasKey = normalize(alias);
        const std::string mapKey = normalize(mapId);
        if (maps_.find(mapKey) == maps_.end()) {
            return;
        }

        aliases_[aliasKey] = mapKey;
    }

    const MapDefinition* find(const std::string& idOrAlias) const {
        const std::string key = normalize(idOrAlias);
        const auto aliasIt = aliases_.find(key);
        if (aliasIt == aliases_.end()) {
            return nullptr;
        }

        const auto mapIt = maps_.find(aliasIt->second);
        if (mapIt == maps_.end()) {
            return nullptr;
        }

        return &mapIt->second;
    }

    std::vector<std::string> mapIds() const {
        std::vector<std::string> ids;
        ids.reserve(maps_.size());

        for (const auto& [_, map] : maps_) {
            ids.push_back(map.id);
        }

        std::sort(ids.begin(), ids.end());
        return ids;
    }

private:
    static std::string normalize(const std::string& value) {
        std::string normalized;
        normalized.reserve(value.size());

        for (char ch : value) {
            if (ch == '\\') {
                normalized.push_back('/');
            } else {
                normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
            }
        }

        return normalized;
    }

    std::unordered_map<std::string, MapDefinition> maps_;
    std::unordered_map<std::string, std::string> aliases_;
};
