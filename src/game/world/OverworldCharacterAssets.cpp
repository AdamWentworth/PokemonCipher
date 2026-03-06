#include "game/world/OverworldCharacterAssets.h"

#include <memory>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace {
void readOptionalStringField(lua_State* lua, const int tableIndex, const char* fieldName, std::string& value) {
    lua_getfield(lua, tableIndex, fieldName);
    if (lua_isstring(lua, -1)) {
        const char* text = lua_tostring(lua, -1);
        value = text ? text : "";
    }
    lua_pop(lua, 1);
}

void readOptionalNumberField(lua_State* lua, const int tableIndex, const char* fieldName, float& value) {
    lua_getfield(lua, tableIndex, fieldName);
    if (lua_isnumber(lua, -1)) {
        value = static_cast<float>(lua_tonumber(lua, -1));
    }
    lua_pop(lua, 1);
}
} // namespace

OverworldCharacterAssets makeDefaultOverworldCharacterAssets() {
    return OverworldCharacterAssets{};
}

bool loadOverworldCharacterAssetsFromLuaFile(
    const std::string& scriptPath,
    OverworldCharacterAssets& outAssets,
    std::string& errorOut
) {
    outAssets = makeDefaultOverworldCharacterAssets();
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
        errorOut = "Overworld character config must return a table.";
        lua_pop(lua.get(), 1);
        return false;
    }

    const int tableIndex = lua_gettop(lua.get());
    readOptionalStringField(lua.get(), tableIndex, "player_animation_path", outAssets.playerAnimationPath);
    readOptionalStringField(lua.get(), tableIndex, "player_sprite_path", outAssets.playerSpritePath);
    readOptionalNumberField(lua.get(), tableIndex, "player_base_draw_height", outAssets.playerBaseDrawHeight);
    readOptionalNumberField(lua.get(), tableIndex, "player_draw_scale", outAssets.playerDrawScale);
    readOptionalStringField(lua.get(), tableIndex, "npc_default_animation_path", outAssets.npcDefaultAnimationPath);
    readOptionalStringField(lua.get(), tableIndex, "npc_default_sprite_path", outAssets.npcDefaultSpritePath);
    readOptionalStringField(lua.get(), tableIndex, "npc_animation_key", outAssets.npcAnimationKey);

    lua_pop(lua.get(), 1);
    return true;
}

