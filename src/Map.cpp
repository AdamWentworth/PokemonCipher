#include "Map.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <tinyxml2.h>

#include "TextureManager.h"

namespace {
std::vector<std::vector<int>> ParseCsvLayer(const char* text, int width, int height) {
    std::vector<std::vector<int>> out(height, std::vector<int>(width, 0));
    if (!text) {
        return out;
    }

    std::stringstream ss(text);
    std::string value;
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            if (!std::getline(ss, value, ',')) {
                return out;
            }
            out[row][col] = value.empty() ? 0 : std::stoi(value);
        }
    }
    return out;
}
} // namespace

void Map::load(const char* path, SDL_Texture* ts) {
    tileset = ts;
    layers.clear();
    colliders.clear();
    spawnPoints.clear();

    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(path) != tinyxml2::XML_SUCCESS) {
        std::cerr << "Failed to load map: " << path << '\n';
        width = height = 0;
        return;
    }

    auto* mapNode = doc.FirstChildElement("map");
    if (!mapNode) {
        std::cerr << "TMX missing <map>: " << path << '\n';
        width = height = 0;
        return;
    }

    width = mapNode->IntAttribute("width");
    height = mapNode->IntAttribute("height");
    tileWidth = mapNode->IntAttribute("tilewidth", 16);
    tileHeight = mapNode->IntAttribute("tileheight", 16);

    for (auto* layer = mapNode->FirstChildElement("layer"); layer; layer = layer->NextSiblingElement("layer")) {
        auto* data = layer->FirstChildElement("data");
        layers.push_back(ParseCsvLayer(data ? data->GetText() : nullptr, width, height));
    }

    for (auto* objectGroup = mapNode->FirstChildElement("objectgroup");
         objectGroup;
         objectGroup = objectGroup->NextSiblingElement("objectgroup")) {
        const char* layerNameAttr = objectGroup->Attribute("name");
        const std::string layerName = layerNameAttr ? layerNameAttr : "";

        const bool isCollisionLayer = layerName.find("Collision") != std::string::npos;
        const bool isSpawnLayer = layerName.find("Spawn") != std::string::npos;

        for (auto* obj = objectGroup->FirstChildElement("object"); obj; obj = obj->NextSiblingElement("object")) {
            if (isCollisionLayer) {
                Collider c;
                c.tag = "wall";
                c.rect.x = obj->FloatAttribute("x");
                c.rect.y = obj->FloatAttribute("y");
                c.rect.w = obj->FloatAttribute("width");
                c.rect.h = obj->FloatAttribute("height");
                colliders.push_back(c);
            }

            if (isSpawnLayer && obj->FirstChildElement("point")) {
                const char* spawnName = obj->Attribute("name");
                if (spawnName && spawnName[0] != '\0') {
                    spawnPoints[spawnName] = Vector2D(obj->FloatAttribute("x"), obj->FloatAttribute("y"));
                }
            }
        }
    }
}

Vector2D Map::getSpawnPoint(const std::string& spawnId, const Vector2D& fallback) const {
    auto it = spawnPoints.find(spawnId);
    if (it != spawnPoints.end()) {
        return it->second;
    }
    return fallback;
}

void Map::draw(const Camera& cam) {
    if (!tileset || width <= 0 || height <= 0 || layers.empty()) {
        return;
    }

    float textureWidth = 0.0f;
    float textureHeight = 0.0f;
    if (!SDL_GetTextureSize(tileset, &textureWidth, &textureHeight) || textureWidth <= 0 || textureHeight <= 0) {
        return;
    }

    const int columns = textureWidth / tileWidth;
    if (columns <= 0) {
        return;
    }

    SDL_FRect src{};
    src.w = static_cast<float>(tileWidth);
    src.h = static_cast<float>(tileHeight);

    SDL_FRect dst{};
    dst.w = static_cast<float>(tileWidth * renderScale);
    dst.h = static_cast<float>(tileHeight * renderScale);

    for (const auto& layer : layers) {
        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {
                const int gid = layer[row][col];
                if (gid <= 0) {
                    continue;
                }

                const int tileIndex = gid - 1;
                const int srcCol = tileIndex % columns;
                const int srcRow = tileIndex / columns;

                src.x = static_cast<float>(srcCol * tileWidth);
                src.y = static_cast<float>(srcRow * tileHeight);

                const float worldX = static_cast<float>(col * tileWidth * renderScale);
                const float worldY = static_cast<float>(row * tileHeight * renderScale);
                dst.x = std::round(worldX - cam.view.x);
                dst.y = std::round(worldY - cam.view.y);

                TextureManager::draw(tileset, src, dst);
            }
        }
    }
}
