#pragma once

#include <string>

#include "game/world/MapRegistry.h"

bool loadMapRegistryFromLuaFile(const std::string& scriptPath, MapRegistry& outRegistry, std::string& errorOut);

