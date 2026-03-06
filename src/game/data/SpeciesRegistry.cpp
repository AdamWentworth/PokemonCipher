#include "game/data/SpeciesRegistry.h"

#include <iostream>
#include <memory>
#include <sstream>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace {
constexpr int kTypeNormal = 0;
constexpr int kTypeFlying = 2;
constexpr int kTypeGround = 4;

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

bool readIntegerField(lua_State* lua, const int tableIndex, const char* fieldName, int& outValue) {
    lua_getfield(lua, tableIndex, fieldName);
    if (!lua_isnumber(lua, -1)) {
        lua_pop(lua, 1);
        return false;
    }

    outValue = static_cast<int>(lua_tointeger(lua, -1));
    lua_pop(lua, 1);
    return true;
}

bool loadMoveSet(lua_State* lua, const int speciesIndex, SpeciesDefinition& outSpecies, std::string& errorOut, const std::size_t speciesTableIndex) {
    lua_getfield(lua, speciesIndex, "moves");
    if (lua_isnil(lua, -1)) {
        lua_pop(lua, 1);
        return true;
    }

    if (!lua_istable(lua, -1)) {
        std::ostringstream error;
        error << "species[" << speciesTableIndex << "].moves must be a table when present.";
        errorOut = error.str();
        lua_pop(lua, 1);
        return false;
    }

    const int movesIndex = lua_gettop(lua);
    const std::size_t moveCount = std::min<std::size_t>(4, lua_rawlen(lua, movesIndex));
    for (std::size_t i = 1; i <= moveCount; ++i) {
        lua_rawgeti(lua, movesIndex, static_cast<lua_Integer>(i));
        if (!lua_istable(lua, -1)) {
            std::ostringstream error;
            error << "species[" << speciesTableIndex << "].moves[" << i << "] must be a table.";
            errorOut = error.str();
            lua_pop(lua, 2);
            return false;
        }

        const int moveIndex = lua_gettop(lua);
        SpeciesMoveDefinition move{};
        readStringField(lua, moveIndex, "name", move.name);
        readIntegerField(lua, moveIndex, "pp", move.pp);
        readIntegerField(lua, moveIndex, "max_pp", move.maxPp);
        readIntegerField(lua, moveIndex, "type_id", move.typeId);
        outSpecies.moves[i - 1] = std::move(move);

        lua_pop(lua, 1);
    }

    lua_pop(lua, 1);
    return true;
}
} // namespace

void SpeciesRegistry::addSpecies(const SpeciesDefinition& species) {
    if (species.id <= 0) {
        return;
    }
    byId_[species.id] = species;
}

const SpeciesDefinition* SpeciesRegistry::find(const int speciesId) const {
    const auto it = byId_.find(speciesId);
    if (it == byId_.end()) {
        return nullptr;
    }

    return &it->second;
}

SpeciesRegistry buildFallbackSpeciesRegistry() {
    SpeciesRegistry registry;

    SpeciesDefinition pidgey{};
    pidgey.id = 16;
    pidgey.name = "PIDGEY";
    pidgey.primaryTypeId = kTypeNormal;
    pidgey.secondaryTypeId = kTypeFlying;
    pidgey.abilityName = "KEEN EYE";
    pidgey.abilityDescription = "Prevents loss of accuracy.";
    pidgey.moves = {{
        {"TACKLE", 35, 35, kTypeNormal},
        {"GROWL", 40, 40, kTypeNormal},
        {"QUICK ATTACK", 30, 30, kTypeNormal},
        {"SAND-ATTACK", 15, 15, kTypeGround},
    }};
    registry.addSpecies(pidgey);

    SpeciesDefinition rattata{};
    rattata.id = 19;
    rattata.name = "RATTATA";
    rattata.primaryTypeId = kTypeNormal;
    rattata.secondaryTypeId = kTypeNormal;
    rattata.abilityName = "RUN AWAY";
    rattata.abilityDescription = "Enables sure getaway.";
    rattata.moves = {{
        {"TACKLE", 35, 35, kTypeNormal},
        {"TAIL WHIP", 30, 30, kTypeNormal},
        {"QUICK ATTACK", 30, 30, kTypeNormal},
        {"SAND-ATTACK", 15, 15, kTypeGround},
    }};
    registry.addSpecies(rattata);

    SpeciesDefinition eevee{};
    eevee.id = 133;
    eevee.name = "EEVEE";
    eevee.primaryTypeId = kTypeNormal;
    eevee.secondaryTypeId = kTypeNormal;
    eevee.abilityName = "RUN AWAY";
    eevee.abilityDescription = "Enables sure getaway.";
    eevee.frontSpritePath = "assets/pokemon/eevee/front.png";
    eevee.iconPath = "assets/pokemon/eevee/icon.png";
    eevee.moves = {{
        {"TACKLE", 35, 35, kTypeNormal},
        {"TAIL WHIP", 30, 30, kTypeNormal},
        {"HELPING HAND", 20, 20, kTypeNormal},
        {"SAND-ATTACK", 15, 15, kTypeGround},
    }};
    registry.addSpecies(eevee);

    return registry;
}

bool loadSpeciesRegistryFromLuaFile(const std::string& scriptPath, SpeciesRegistry& outRegistry, std::string& errorOut) {
    outRegistry = SpeciesRegistry{};
    errorOut.clear();

    std::unique_ptr<lua_State, decltype(&lua_close)> lua(luaL_newstate(), lua_close);
    if (!lua) {
        errorOut = "Failed to create Lua state.";
        return false;
    }

    luaL_openlibs(lua.get());
    if (luaL_loadfile(lua.get(), scriptPath.c_str()) != LUA_OK) {
        const char* error = lua_tostring(lua.get(), -1);
        errorOut = error ? error : "Unknown Lua load error.";
        lua_pop(lua.get(), 1);
        return false;
    }

    if (lua_pcall(lua.get(), 0, 1, 0) != LUA_OK) {
        const char* error = lua_tostring(lua.get(), -1);
        errorOut = error ? error : "Unknown Lua runtime error.";
        lua_pop(lua.get(), 1);
        return false;
    }

    if (!lua_istable(lua.get(), -1)) {
        errorOut = "Species registry script must return a table.";
        lua_pop(lua.get(), 1);
        return false;
    }

    const int rootIndex = lua_gettop(lua.get());
    lua_getfield(lua.get(), rootIndex, "species");
    if (!lua_istable(lua.get(), -1)) {
        errorOut = "Species registry config must define a 'species' table.";
        lua_pop(lua.get(), 2);
        return false;
    }

    const int speciesIndex = lua_gettop(lua.get());
    const std::size_t speciesCount = lua_rawlen(lua.get(), speciesIndex);
    for (std::size_t i = 1; i <= speciesCount; ++i) {
        lua_rawgeti(lua.get(), speciesIndex, static_cast<lua_Integer>(i));
        if (!lua_istable(lua.get(), -1)) {
            std::ostringstream error;
            error << "species[" << i << "] must be a table.";
            errorOut = error.str();
            lua_pop(lua.get(), 3);
            return false;
        }

        const int oneSpeciesIndex = lua_gettop(lua.get());
        SpeciesDefinition species{};
        if (!readIntegerField(lua.get(), oneSpeciesIndex, "id", species.id)) {
            std::ostringstream error;
            error << "species[" << i << "] is missing required integer field 'id'.";
            errorOut = error.str();
            lua_pop(lua.get(), 3);
            return false;
        }

        readStringField(lua.get(), oneSpeciesIndex, "name", species.name);
        readIntegerField(lua.get(), oneSpeciesIndex, "primary_type_id", species.primaryTypeId);
        if (!readIntegerField(lua.get(), oneSpeciesIndex, "secondary_type_id", species.secondaryTypeId)) {
            species.secondaryTypeId = species.primaryTypeId;
        }
        readStringField(lua.get(), oneSpeciesIndex, "ability_name", species.abilityName);
        readStringField(lua.get(), oneSpeciesIndex, "ability_description", species.abilityDescription);
        readStringField(lua.get(), oneSpeciesIndex, "front_sprite_path", species.frontSpritePath);
        readStringField(lua.get(), oneSpeciesIndex, "icon_path", species.iconPath);

        if (!loadMoveSet(lua.get(), oneSpeciesIndex, species, errorOut, i)) {
            lua_pop(lua.get(), 3);
            return false;
        }

        outRegistry.addSpecies(species);
        lua_pop(lua.get(), 1);
    }

    lua_pop(lua.get(), 2);
    return true;
}

const SpeciesRegistry& getSpeciesRegistry() {
    static const SpeciesRegistry registry = []() {
        SpeciesRegistry loaded{};
        std::string error;
        if (loadSpeciesRegistryFromLuaFile("assets/config/pokemon/species_registry.lua", loaded, error)) {
            return loaded;
        }

        if (!error.empty()) {
            std::cout << "Species registry config load failed; using fallback: " << error << '\n';
        }
        return buildFallbackSpeciesRegistry();
    }();

    return registry;
}
