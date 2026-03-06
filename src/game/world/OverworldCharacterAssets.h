#pragma once

#include <string>

struct OverworldCharacterAssets {
    std::string playerAnimationPath = "assets/animations/wes_overworld.xml";
    std::string playerSpritePath = "assets/characters/wes/wes_overworld.png";
    float playerBaseDrawHeight = 32.0f;
    float playerDrawScale = 0.704f;

    std::string npcDefaultAnimationPath = "assets/animations/red_overworld.xml";
    std::string npcDefaultSpritePath = "assets/characters/oak/oak_overworld.png";
    std::string npcAnimationKey = "npc_default";
};

OverworldCharacterAssets makeDefaultOverworldCharacterAssets();
bool loadOverworldCharacterAssetsFromLuaFile(
    const std::string& scriptPath,
    OverworldCharacterAssets& outAssets,
    std::string& errorOut
);

