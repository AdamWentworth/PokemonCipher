#include "game/ui/PartyMenuLayout.h"

#include <algorithm>
#include <memory>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace {
void readOptionalNumberField(lua_State* lua, const int tableIndex, const char* fieldName, float& value) {
    lua_getfield(lua, tableIndex, fieldName);
    if (lua_isnumber(lua, -1)) {
        value = static_cast<float>(lua_tonumber(lua, -1));
    }
    lua_pop(lua, 1);
}

void readOptionalRectField(lua_State* lua, const int tableIndex, const char* fieldName, SDL_FRect& rect) {
    lua_getfield(lua, tableIndex, fieldName);
    if (lua_istable(lua, -1)) {
        const int rectIndex = lua_gettop(lua);
        readOptionalNumberField(lua, rectIndex, "x", rect.x);
        readOptionalNumberField(lua, rectIndex, "y", rect.y);
        readOptionalNumberField(lua, rectIndex, "w", rect.w);
        readOptionalNumberField(lua, rectIndex, "h", rect.h);
    }
    lua_pop(lua, 1);
}

void readOptionalRectArrayField(
    lua_State* lua,
    const int tableIndex,
    const char* fieldName,
    std::array<SDL_FRect, 6>& outRects
) {
    lua_getfield(lua, tableIndex, fieldName);
    if (!lua_istable(lua, -1)) {
        lua_pop(lua, 1);
        return;
    }

    const int arrayIndex = lua_gettop(lua);
    const std::size_t count = std::min<std::size_t>(outRects.size(), lua_rawlen(lua, arrayIndex));
    for (std::size_t i = 0; i < count; ++i) {
        lua_rawgeti(lua, arrayIndex, static_cast<lua_Integer>(i + 1));
        if (lua_istable(lua, -1)) {
            const int rectIndex = lua_gettop(lua);
            readOptionalNumberField(lua, rectIndex, "x", outRects[i].x);
            readOptionalNumberField(lua, rectIndex, "y", outRects[i].y);
            readOptionalNumberField(lua, rectIndex, "w", outRects[i].w);
            readOptionalNumberField(lua, rectIndex, "h", outRects[i].h);
        }
        lua_pop(lua, 1);
    }

    lua_pop(lua, 1);
}

void readOptionalPointArrayField(
    lua_State* lua,
    const int tableIndex,
    const char* fieldName,
    std::array<SDL_FPoint, 6>& outPoints
) {
    lua_getfield(lua, tableIndex, fieldName);
    if (!lua_istable(lua, -1)) {
        lua_pop(lua, 1);
        return;
    }

    const int arrayIndex = lua_gettop(lua);
    const std::size_t count = std::min<std::size_t>(outPoints.size(), lua_rawlen(lua, arrayIndex));
    for (std::size_t i = 0; i < count; ++i) {
        lua_rawgeti(lua, arrayIndex, static_cast<lua_Integer>(i + 1));
        if (lua_istable(lua, -1)) {
            const int pointIndex = lua_gettop(lua);
            readOptionalNumberField(lua, pointIndex, "x", outPoints[i].x);
            readOptionalNumberField(lua, pointIndex, "y", outPoints[i].y);
        }
        lua_pop(lua, 1);
    }

    lua_pop(lua, 1);
}
} // namespace

PartyMenuLayout makeDefaultPartyMenuLayout() {
    return PartyMenuLayout{};
}

bool loadPartyMenuLayoutFromLuaFile(const std::string& scriptPath, PartyMenuLayout& outLayout, std::string& errorOut) {
    outLayout = makeDefaultPartyMenuLayout();
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
        errorOut = "Party menu layout script must return a table.";
        lua_pop(lua.get(), 1);
        return false;
    }

    const int tableIndex = lua_gettop(lua.get());
    readOptionalNumberField(lua.get(), tableIndex, "design_width", outLayout.designWidth);
    readOptionalNumberField(lua.get(), tableIndex, "design_height", outLayout.designHeight);
    readOptionalNumberField(lua.get(), tableIndex, "text_baseline_y_offset", outLayout.textBaselineYOffset);

    readOptionalRectArrayField(lua.get(), tableIndex, "slot_rects", outLayout.slotRects);
    readOptionalPointArrayField(lua.get(), tableIndex, "pokeball_positions", outLayout.pokeballPositions);
    readOptionalPointArrayField(lua.get(), tableIndex, "pokemon_icon_positions", outLayout.pokemonIconPositions);
    readOptionalRectField(lua.get(), tableIndex, "message_rect", outLayout.messageRect);
    readOptionalRectField(lua.get(), tableIndex, "cancel_rect", outLayout.cancelRect);
    readOptionalRectField(lua.get(), tableIndex, "command_rect", outLayout.commandRect);

    readOptionalNumberField(lua.get(), tableIndex, "name_text_offset_x", outLayout.nameTextOffsetX);
    readOptionalNumberField(lua.get(), tableIndex, "name_text_main_y_offset", outLayout.nameTextMainYOffset);
    readOptionalNumberField(lua.get(), tableIndex, "name_text_other_y_offset", outLayout.nameTextOtherYOffset);
    readOptionalNumberField(lua.get(), tableIndex, "level_text_main_x", outLayout.levelTextMainX);
    readOptionalNumberField(lua.get(), tableIndex, "level_text_other_x", outLayout.levelTextOtherX);
    readOptionalNumberField(lua.get(), tableIndex, "level_text_main_y_offset", outLayout.levelTextMainYOffset);
    readOptionalNumberField(lua.get(), tableIndex, "level_text_other_y_offset", outLayout.levelTextOtherYOffset);

    readOptionalNumberField(lua.get(), tableIndex, "hp_bar_main_x", outLayout.hpBarMainX);
    readOptionalNumberField(lua.get(), tableIndex, "hp_bar_other_x", outLayout.hpBarOtherX);
    readOptionalNumberField(lua.get(), tableIndex, "hp_bar_main_y", outLayout.hpBarMainY);
    readOptionalNumberField(lua.get(), tableIndex, "hp_bar_other_y", outLayout.hpBarOtherY);
    readOptionalNumberField(lua.get(), tableIndex, "hp_bar_width", outLayout.hpBarWidth);
    readOptionalNumberField(lua.get(), tableIndex, "hp_bar_height", outLayout.hpBarHeight);
    readOptionalNumberField(lua.get(), tableIndex, "hp_text_main_x", outLayout.hpTextMainX);
    readOptionalNumberField(lua.get(), tableIndex, "hp_text_other_x", outLayout.hpTextOtherX);
    readOptionalNumberField(lua.get(), tableIndex, "hp_text_main_y", outLayout.hpTextMainY);
    readOptionalNumberField(lua.get(), tableIndex, "hp_text_other_y", outLayout.hpTextOtherY);

    readOptionalNumberField(lua.get(), tableIndex, "prompt_text_x_offset", outLayout.promptTextXOffset);
    readOptionalNumberField(lua.get(), tableIndex, "prompt_text_y_offset", outLayout.promptTextYOffset);
    readOptionalNumberField(lua.get(), tableIndex, "command_arrow_x_offset", outLayout.commandArrowXOffset);
    readOptionalNumberField(lua.get(), tableIndex, "command_label_x_offset", outLayout.commandLabelXOffset);
    readOptionalNumberField(lua.get(), tableIndex, "command_line_start_y_offset", outLayout.commandLineStartYOffset);
    readOptionalNumberField(lua.get(), tableIndex, "command_line_step", outLayout.commandLineStep);

    lua_pop(lua.get(), 1);
    return true;
}

