#include "Map.h"
#include "TextureManager.h"
#include <sstream>
#include <string>
#include <tinyxml2.h>

void Map::load(const char *path, SDL_Texture *ts) {
    
    tileset = ts;
    tinyxml2::XMLDocument doc;
    doc.LoadFile(path);

    auto* mapNode = doc.FirstChildElement("map");
    width = mapNode->IntAttribute("width");
    height = mapNode->IntAttribute("height");
    colliders.clear();
    itemSpawnPoints.clear();

    auto* layer = mapNode->FirstChildElement("layer");
    auto* data = layer->FirstChildElement("data");
    std::string csv = data->GetText();
    std::stringstream ss(csv);
    tileData = std::vector(height, std::vector<int>(width));
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            std::string val;
            if (!std::getline(ss, val, ',')) break;
            tileData[i][j] = std::stoi(val);
        }
    }
    // Parse object groups by their "name" value, not by XML order.
    // This keeps loading stable even if groups are reordered or extra groups are added.
    for (auto* objectGroup = mapNode->FirstChildElement("objectgroup");
        objectGroup != nullptr;
        objectGroup = objectGroup->NextSiblingElement("objectgroup")) {

        const char* layerName = objectGroup->Attribute("name");
        if (!layerName) continue;
        const std::string groupName(layerName);
        // Use substring checks so variants like
        // "Item Spawn Points" and "Item Layer" both match.
        const bool isCollisionLayer = groupName.find("Collision") != std::string::npos;
        const bool isItemLayer = groupName.find("Item") != std::string::npos;

        // Collision layers contribute rectangle colliders.
        if (isCollisionLayer) {
            for (auto* obj = objectGroup->FirstChildElement("object");
                obj != nullptr;
                obj = obj->NextSiblingElement("object")) {

                Collider c;
                c.rect.x = obj->FloatAttribute("x");
                c.rect.y = obj->FloatAttribute("y");
                c.rect.w = obj->FloatAttribute("width");
                c.rect.h = obj->FloatAttribute("height");
                colliders.push_back(c);
            }
        }

        // Item layers contribute point spawn positions.
        if (isItemLayer) {
            for (auto* obj = objectGroup->FirstChildElement("object");
                obj != nullptr;
                obj = obj->NextSiblingElement("object")) {

                if (!obj->FirstChildElement("point")) continue;
                itemSpawnPoints.push_back(Vector2D(obj->FloatAttribute("x"), obj->FloatAttribute("y")));
            }
        }
    }
}



void Map::draw(const Camera &cam) {

    SDL_FRect src{}, dest{};

    dest.w = dest.h = 32;

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int type = tileData[row][col];

            float worldX = static_cast<float>(col) * dest.w;
            float worldY = static_cast<float>(row) * dest.h;

            dest.x = std::round(worldX - cam.view.x);
            dest.y = std::round(worldY - cam.view.y);

            switch (type) {
                //dirt
                case 1: 
                    src.x = 0;
                    src.y = 0;
                    src.w = 32;
                    src.h = 32;
                    break;
                //grass
                case 2: 
                    src.x = 32;
                    src.y = 0;
                    src.w = 32;
                    src.h = 32;
                    break;
                //water
                case 4: 
                    src.x = 32;
                    src.y = 32;
                    src.w = 32;
                    src.h = 32;
                    break;
                //red
                default: 
                    break;
            }
            TextureManager::draw(tileset, src, dest);
        }
    }
}
