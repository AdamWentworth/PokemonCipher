#include "game/scripting/LuaOverworldScriptLoader.h"

#include <memory>
#include <sstream>
#include <string>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace {
bool readStringArg(
    lua_State* lua,
    const int commandIndex,
    const int argIndex,
    const std::size_t commandNumber,
    std::string& outValue,
    std::string& errorOut
) {
    lua_rawgeti(lua, commandIndex, argIndex);
    if (!lua_isstring(lua, -1)) {
        std::ostringstream error;
        error << "Command #" << commandNumber << " expects string argument #" << argIndex << '.';
        errorOut = error.str();
        lua_pop(lua, 1);
        return false;
    }

    const char* value = lua_tostring(lua, -1);
    outValue = value ? value : "";
    lua_pop(lua, 1);
    return true;
}

bool readNumberArg(
    lua_State* lua,
    const int commandIndex,
    const int argIndex,
    const std::size_t commandNumber,
    float& outValue,
    std::string& errorOut
) {
    lua_rawgeti(lua, commandIndex, argIndex);
    if (!lua_isnumber(lua, -1)) {
        std::ostringstream error;
        error << "Command #" << commandNumber << " expects number argument #" << argIndex << '.';
        errorOut = error.str();
        lua_pop(lua, 1);
        return false;
    }

    outValue = static_cast<float>(lua_tonumber(lua, -1));
    lua_pop(lua, 1);
    return true;
}

bool readIntegerArg(
    lua_State* lua,
    const int commandIndex,
    const int argIndex,
    const std::size_t commandNumber,
    int& outValue,
    std::string& errorOut
) {
    lua_rawgeti(lua, commandIndex, argIndex);
    if (!lua_isinteger(lua, -1)) {
        std::ostringstream error;
        error << "Command #" << commandNumber << " expects integer argument #" << argIndex << '.';
        errorOut = error.str();
        lua_pop(lua, 1);
        return false;
    }

    outValue = static_cast<int>(lua_tointeger(lua, -1));
    lua_pop(lua, 1);
    return true;
}

bool readBoolArg(
    lua_State* lua,
    const int commandIndex,
    const int argIndex,
    const std::size_t commandNumber,
    bool& outValue,
    std::string& errorOut
) {
    lua_rawgeti(lua, commandIndex, argIndex);
    if (!lua_isboolean(lua, -1)) {
        std::ostringstream error;
        error << "Command #" << commandNumber << " expects boolean argument #" << argIndex << '.';
        errorOut = error.str();
        lua_pop(lua, 1);
        return false;
    }

    outValue = lua_toboolean(lua, -1) != 0;
    lua_pop(lua, 1);
    return true;
}
} // namespace

bool loadOverworldScriptFromLuaFile(
    const std::string& scriptId,
    const std::string& scriptPath,
    OverworldScript& outScript,
    std::string& errorOut
) {
    outScript = OverworldScript{};
    outScript.id = scriptId;
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
        errorOut = "Script must return a table of commands.";
        lua_pop(lua.get(), 1);
        return false;
    }

    const int scriptTableIndex = lua_gettop(lua.get());
    const std::size_t commandCount = lua_rawlen(lua.get(), scriptTableIndex);
    outScript.commands.reserve(commandCount);

    for (std::size_t i = 1; i <= commandCount; ++i) {
        lua_rawgeti(lua.get(), scriptTableIndex, static_cast<lua_Integer>(i));
        if (!lua_istable(lua.get(), -1)) {
            std::ostringstream error;
            error << "Command #" << i << " must be a table.";
            errorOut = error.str();
            lua_pop(lua.get(), 1);
            return false;
        }

        const int commandIndex = lua_gettop(lua.get());

        std::string op;
        if (!readStringArg(lua.get(), commandIndex, 1, i, op, errorOut)) {
            lua_pop(lua.get(), 1);
            return false;
        }

        if (op == "log") {
            std::string message;
            if (!readStringArg(lua.get(), commandIndex, 2, i, message, errorOut)) {
                lua_pop(lua.get(), 1);
                return false;
            }
            outScript.commands.push_back(OverworldScriptCommand::Log(message));
        } else if (op == "say") {
            std::string speaker;
            std::string message;
            if (!readStringArg(lua.get(), commandIndex, 2, i, speaker, errorOut) ||
                !readStringArg(lua.get(), commandIndex, 3, i, message, errorOut)) {
                lua_pop(lua.get(), 1);
                return false;
            }
            outScript.commands.push_back(OverworldScriptCommand::Dialogue(speaker, message));
        } else if (op == "wait") {
            float seconds = 0.0f;
            if (!readNumberArg(lua.get(), commandIndex, 2, i, seconds, errorOut)) {
                lua_pop(lua.get(), 1);
                return false;
            }
            outScript.commands.push_back(OverworldScriptCommand::Wait(seconds));
        } else if (op == "lock_input") {
            outScript.commands.push_back(OverworldScriptCommand::LockInput());
        } else if (op == "unlock_input") {
            outScript.commands.push_back(OverworldScriptCommand::UnlockInput());
        } else if (op == "set_flag") {
            std::string key;
            bool value = false;
            if (!readStringArg(lua.get(), commandIndex, 2, i, key, errorOut) ||
                !readBoolArg(lua.get(), commandIndex, 3, i, value, errorOut)) {
                lua_pop(lua.get(), 1);
                return false;
            }
            outScript.commands.push_back(OverworldScriptCommand::SetFlag(key, value));
        } else if (op == "set_var") {
            std::string key;
            int value = 0;
            if (!readStringArg(lua.get(), commandIndex, 2, i, key, errorOut) ||
                !readIntegerArg(lua.get(), commandIndex, 3, i, value, errorOut)) {
                lua_pop(lua.get(), 1);
                return false;
            }
            outScript.commands.push_back(OverworldScriptCommand::SetVar(key, value));
        } else if (op == "clear_party") {
            outScript.commands.push_back(OverworldScriptCommand::ClearParty());
        } else if (op == "add_party") {
            int speciesId = 0;
            int level = 1;
            bool isPartner = false;
            if (!readIntegerArg(lua.get(), commandIndex, 2, i, speciesId, errorOut) ||
                !readIntegerArg(lua.get(), commandIndex, 3, i, level, errorOut)) {
                lua_pop(lua.get(), 1);
                return false;
            }

            lua_rawgeti(lua.get(), commandIndex, 4);
            if (lua_isboolean(lua.get(), -1)) {
                isPartner = lua_toboolean(lua.get(), -1) != 0;
            }
            lua_pop(lua.get(), 1);

            outScript.commands.push_back(OverworldScriptCommand::AddPartyPokemon(speciesId, level, isPartner));
        } else if (op == "warp_spawn") {
            std::string mapId;
            std::string spawnId = "default";
            if (!readStringArg(lua.get(), commandIndex, 2, i, mapId, errorOut)) {
                lua_pop(lua.get(), 1);
                return false;
            }

            lua_rawgeti(lua.get(), commandIndex, 3);
            if (lua_isstring(lua.get(), -1)) {
                const char* value = lua_tostring(lua.get(), -1);
                spawnId = value ? value : "default";
            }
            lua_pop(lua.get(), 1);

            outScript.commands.push_back(OverworldScriptCommand::WarpToSpawn(mapId, spawnId));
        } else if (op == "warp_xy") {
            std::string mapId;
            float x = 0.0f;
            float y = 0.0f;
            if (!readStringArg(lua.get(), commandIndex, 2, i, mapId, errorOut) ||
                !readNumberArg(lua.get(), commandIndex, 3, i, x, errorOut) ||
                !readNumberArg(lua.get(), commandIndex, 4, i, y, errorOut)) {
                lua_pop(lua.get(), 1);
                return false;
            }
            outScript.commands.push_back(OverworldScriptCommand::WarpToPosition(mapId, Vector2D(x, y)));
        } else if (op == "teleport") {
            float x = 0.0f;
            float y = 0.0f;
            if (!readNumberArg(lua.get(), commandIndex, 2, i, x, errorOut) ||
                !readNumberArg(lua.get(), commandIndex, 3, i, y, errorOut)) {
                lua_pop(lua.get(), 1);
                return false;
            }
            outScript.commands.push_back(OverworldScriptCommand::TeleportToPosition(Vector2D(x, y)));
        } else {
            std::ostringstream error;
            error << "Unknown command op '" << op << "' at command #" << i << '.';
            errorOut = error.str();
            lua_pop(lua.get(), 1);
            return false;
        }

        lua_pop(lua.get(), 1);
    }

    lua_pop(lua.get(), 1);
    return true;
}
