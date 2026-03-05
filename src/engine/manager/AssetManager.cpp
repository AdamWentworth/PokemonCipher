#include "AssetManager.h"

#include <stdexcept>

#include <tinyxml2.h>

std::unordered_map<std::string, Animation> AssetManager::animations_;

void AssetManager::loadAnimation(const std::string& clipName, const char* path) {
    animations_[clipName] = loadAnimationFromXML(path);
}

bool AssetManager::hasAnimation(const std::string& clipName) {
    return animations_.find(clipName) != animations_.end();
}

const Animation& AssetManager::getAnimation(const std::string& clipName) {
    const auto it = animations_.find(clipName);
    if (it == animations_.end()) {
        throw std::runtime_error("Animation not loaded: " + clipName);
    }

    return it->second;
}

Animation AssetManager::loadAnimationFromXML(const char* path) {
    Animation animation;

    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(path) != tinyxml2::XML_SUCCESS) {
        return animation;
    }

    const auto* root = doc.FirstChildElement("animations");
    if (!root) {
        return animation;
    }

    for (const auto* clipElement = root->FirstChildElement();
         clipElement != nullptr;
         clipElement = clipElement->NextSiblingElement()) {

        const std::string clipName = clipElement->Name();
        AnimationClip clip;

        for (const auto* frameElement = clipElement->FirstChildElement("frame");
             frameElement != nullptr;
             frameElement = frameElement->NextSiblingElement("frame")) {

            SDL_FRect frame{};
            frameElement->QueryFloatAttribute("x", &frame.x);
            frameElement->QueryFloatAttribute("y", &frame.y);
            frameElement->QueryFloatAttribute("w", &frame.w);
            frameElement->QueryFloatAttribute("h", &frame.h);
            clip.frameIndices.push_back(frame);
        }

        animation.clips[clipName] = clip;
    }

    if (!animation.clips.empty()) {
        animation.currentClip = animation.clips.begin()->first;
    }

    return animation;
}
