#pragma once

#include "Map.h"

namespace MapLoader {
// TMX parsing has its own reason to change, so this loader fills Map data and
// leaves Map.cpp focused on rendering that already-loaded map.
void loadInto(Map& map, const char* path, SDL_Texture* tileset);
}
