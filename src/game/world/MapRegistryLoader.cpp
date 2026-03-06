#include "game/world/MapRegistryLoader.h"

#include <memory>
#include <sstream>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace {
bool readStringField(lua_State* lua, const int tableIndex, const char* fieldName, std::string& outValue) {
    lua_getfield(lua, tableIndex, fieldName);
    if (!lua_isstring(lua, -1)) {
        lua_pop(lua, 1);
        return false;
    }

    const char* value = lua_tostring(lua, -1);
    outValue = value ? value : "";
    lua_pop(lua, 1);
    return true;
}

bool loadMaps(lua_State* lua, const int rootIndex, MapRegistry& outRegistry, std::string& errorOut) {
    lua_getfield(lua, rootIndex, "maps");
    if (!lua_istable(lua, -1)) {
        errorOut = "Map registry config must define a 'maps' table.";
        lua_pop(lua, 1);
        return false;
    }

    const int mapsIndex = lua_gettop(lua);
    const std::size_t mapCount = lua_rawlen(lua, mapsIndex);
    if (mapCount == 0) {
        errorOut = "Map registry config has no map entries.";
        lua_pop(lua, 1);
        return false;
    }

    for (std::size_t i = 1; i <= mapCount; ++i) {
        lua_rawgeti(lua, mapsIndex, static_cast<lua_Integer>(i));
        if (!lua_istable(lua, -1)) {
            std::ostringstream error;
            error << "maps[" << i << "] must be a table.";
            errorOut = error.str();
            lua_pop(lua, 2);
            return false;
        }

        const int mapIndex = lua_gettop(lua);
        MapDefinition map{};
        if (!readStringField(lua, mapIndex, "id", map.id) ||
            !readStringField(lua, mapIndex, "map_path", map.mapPath) ||
            !readStringField(lua, mapIndex, "base_tileset_path", map.baseTilesetPath)) {
            std::ostringstream error;
            error << "maps[" << i << "] is missing required fields (id/map_path/base_tileset_path).";
            errorOut = error.str();
            lua_pop(lua, 2);
            return false;
        }

        if (!readStringField(lua, mapIndex, "cover_tileset_path", map.coverTilesetPath)) {
            map.coverTilesetPath.clear();
        }

        outRegistry.addMap(map);
        lua_pop(lua, 1);
    }

    lua_pop(lua, 1);
    return true;
}

bool loadAliases(lua_State* lua, const int rootIndex, MapRegistry& outRegistry, std::string& errorOut) {
    lua_getfield(lua, rootIndex, "aliases");
    if (lua_isnil(lua, -1)) {
        lua_pop(lua, 1);
        return true;
    }

    if (!lua_istable(lua, -1)) {
        errorOut = "Map registry config field 'aliases' must be a table when present.";
        lua_pop(lua, 1);
        return false;
    }

    const int aliasesIndex = lua_gettop(lua);
    const std::size_t aliasCount = lua_rawlen(lua, aliasesIndex);
    for (std::size_t i = 1; i <= aliasCount; ++i) {
        lua_rawgeti(lua, aliasesIndex, static_cast<lua_Integer>(i));
        if (!lua_istable(lua, -1)) {
            std::ostringstream error;
            error << "aliases[" << i << "] must be a table.";
            errorOut = error.str();
            lua_pop(lua, 2);
            return false;
        }

        const int aliasIndex = lua_gettop(lua);
        std::string alias;
        std::string mapId;
        if (!readStringField(lua, aliasIndex, "alias", alias) || !readStringField(lua, aliasIndex, "map_id", mapId)) {
            std::ostringstream error;
            error << "aliases[" << i << "] must contain alias/map_id strings.";
            errorOut = error.str();
            lua_pop(lua, 2);
            return false;
        }

        outRegistry.addAlias(alias, mapId);
        lua_pop(lua, 1);
    }

    lua_pop(lua, 1);
    return true;
}
} // namespace

bool loadMapRegistryFromLuaFile(const std::string& scriptPath, MapRegistry& outRegistry, std::string& errorOut) {
    outRegistry = MapRegistry{};
    errorOut.clear();

    std::unique_ptr<lua_State, decltype(&lua_close)> lua(luaL_newstate(), lua_close);
    if (!lua) {
        errorOut = "Failed to create Lua state.";
        return false;
    }

    luaL_openlibs(lua.get());

    if (luaL_loadfile(lua.get(), scriptPath.c_str()) != LUA_OK) {
        const char* errorMessage = lua_tostring(lua.get(), -1);
        errorOut = errorMessage ? errorMessage : "Unknown Lua load error.";
        lua_pop(lua.get(), 1);
        return false;
    }

    if (lua_pcall(lua.get(), 0, 1, 0) != LUA_OK) {
        const char* errorMessage = lua_tostring(lua.get(), -1);
        errorOut = errorMessage ? errorMessage : "Unknown Lua runtime error.";
        lua_pop(lua.get(), 1);
        return false;
    }

    if (!lua_istable(lua.get(), -1)) {
        errorOut = "Map registry script must return a table.";
        lua_pop(lua.get(), 1);
        return false;
    }

    const int rootIndex = lua_gettop(lua.get());
    if (!loadMaps(lua.get(), rootIndex, outRegistry, errorOut)) {
        lua_pop(lua.get(), 1);
        return false;
    }

    if (!loadAliases(lua.get(), rootIndex, outRegistry, errorOut)) {
        lua_pop(lua.get(), 1);
        return false;
    }

    lua_pop(lua.get(), 1);
    return true;
}

