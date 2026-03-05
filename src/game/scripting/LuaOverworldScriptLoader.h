#pragma once

#include <string>

#include "game/events/OverworldScript.h"

bool loadOverworldScriptFromLuaFile(
    const std::string& scriptId,
    const std::string& scriptPath,
    OverworldScript& outScript,
    std::string& errorOut
);
