#pragma once

#include <string>
#include <unordered_map>

#include "engine/ecs/Component.h"

class AssetManager {
public:
    static void loadAnimation(const std::string& clipName, const char* path);
    static bool hasAnimation(const std::string& clipName);
    static const Animation& getAnimation(const std::string& clipName);

private:
    static Animation loadAnimationFromXML(const char* path);

    static std::unordered_map<std::string, Animation> animations_;
};
