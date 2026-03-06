#include "game/ui/PokemonSummaryLayout.h"

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

void readOptionalIntegerField(lua_State* lua, const int tableIndex, const char* fieldName, int& value) {
    lua_getfield(lua, tableIndex, fieldName);
    if (lua_isinteger(lua, -1)) {
        value = static_cast<int>(lua_tointeger(lua, -1));
    }
    lua_pop(lua, 1);
}

void readOptionalBoolField(lua_State* lua, const int tableIndex, const char* fieldName, bool& value) {
    lua_getfield(lua, tableIndex, fieldName);
    if (lua_isboolean(lua, -1)) {
        value = lua_toboolean(lua, -1) != 0;
    }
    lua_pop(lua, 1);
}
} // namespace

PokemonSummaryLayout makeDefaultPokemonSummaryLayout() {
    return PokemonSummaryLayout{};
}

bool loadPokemonSummaryLayoutFromLuaFile(
    const std::string& scriptPath,
    PokemonSummaryLayout& outLayout,
    std::string& errorOut
) {
    outLayout = makeDefaultPokemonSummaryLayout();
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
        errorOut = "Summary layout script must return a table.";
        lua_pop(lua.get(), 1);
        return false;
    }

    const int tableIndex = lua_gettop(lua.get());
    readOptionalNumberField(lua.get(), tableIndex, "design_width", outLayout.designWidth);
    readOptionalNumberField(lua.get(), tableIndex, "design_height", outLayout.designHeight);
    readOptionalNumberField(lua.get(), tableIndex, "text_baseline_y_offset", outLayout.textBaselineYOffset);
    readOptionalNumberField(lua.get(), tableIndex, "text_scale_multiplier", outLayout.textScaleMultiplier);
    readOptionalBoolField(lua.get(), tableIndex, "draw_gauge_background", outLayout.drawGaugeBackground);

    readOptionalNumberField(lua.get(), tableIndex, "win_page_name_x", outLayout.windowPageName.x);
    readOptionalNumberField(lua.get(), tableIndex, "win_page_name_y", outLayout.windowPageName.y);
    readOptionalNumberField(lua.get(), tableIndex, "win_controls_x", outLayout.windowControls.x);
    readOptionalNumberField(lua.get(), tableIndex, "win_controls_y", outLayout.windowControls.y);
    readOptionalNumberField(lua.get(), tableIndex, "win_level_nick_x", outLayout.windowLevelNick.x);
    readOptionalNumberField(lua.get(), tableIndex, "win_level_nick_y", outLayout.windowLevelNick.y);
    readOptionalNumberField(lua.get(), tableIndex, "win_info_right_x", outLayout.windowInfoRightPane.x);
    readOptionalNumberField(lua.get(), tableIndex, "win_info_right_y", outLayout.windowInfoRightPane.y);
    readOptionalNumberField(lua.get(), tableIndex, "win_info_memo_x", outLayout.windowInfoTrainerMemo.x);
    readOptionalNumberField(lua.get(), tableIndex, "win_info_memo_y", outLayout.windowInfoTrainerMemo.y);
    readOptionalNumberField(lua.get(), tableIndex, "win_skills_right_x", outLayout.windowSkillsRightPane.x);
    readOptionalNumberField(lua.get(), tableIndex, "win_skills_right_y", outLayout.windowSkillsRightPane.y);
    readOptionalNumberField(lua.get(), tableIndex, "win_skills_exp_x", outLayout.windowSkillsExpText.x);
    readOptionalNumberField(lua.get(), tableIndex, "win_skills_exp_y", outLayout.windowSkillsExpText.y);
    readOptionalNumberField(lua.get(), tableIndex, "win_skills_ability_x", outLayout.windowSkillsAbility.x);
    readOptionalNumberField(lua.get(), tableIndex, "win_skills_ability_y", outLayout.windowSkillsAbility.y);
    readOptionalNumberField(lua.get(), tableIndex, "win_moves_right_x", outLayout.windowMovesRightPane.x);
    readOptionalNumberField(lua.get(), tableIndex, "win_moves_right_y", outLayout.windowMovesRightPane.y);
    readOptionalNumberField(lua.get(), tableIndex, "win_moves_type_x", outLayout.windowMovesTypePane.x);
    readOptionalNumberField(lua.get(), tableIndex, "win_moves_type_y", outLayout.windowMovesTypePane.y);

    readOptionalNumberField(lua.get(), tableIndex, "header_title_x", outLayout.headerTitleX);
    readOptionalNumberField(lua.get(), tableIndex, "header_title_y", outLayout.headerTitleY);
    readOptionalNumberField(lua.get(), tableIndex, "header_controls_align_width", outLayout.controlsAlignWidth);
    readOptionalNumberField(lua.get(), tableIndex, "header_controls_y", outLayout.controlsY);

    readOptionalNumberField(lua.get(), tableIndex, "level_text_x", outLayout.levelTextX);
    readOptionalNumberField(lua.get(), tableIndex, "species_text_x", outLayout.speciesTextX);
    readOptionalNumberField(lua.get(), tableIndex, "gender_symbol_x", outLayout.genderSymbolX);
    readOptionalNumberField(lua.get(), tableIndex, "level_species_y", outLayout.levelSpeciesY);
    readOptionalNumberField(lua.get(), tableIndex, "mon_ball_icon_x", outLayout.monBallIconRect.x);
    readOptionalNumberField(lua.get(), tableIndex, "mon_ball_icon_y", outLayout.monBallIconRect.y);
    readOptionalNumberField(lua.get(), tableIndex, "mon_ball_icon_w", outLayout.monBallIconRect.w);
    readOptionalNumberField(lua.get(), tableIndex, "mon_ball_icon_h", outLayout.monBallIconRect.h);
    readOptionalNumberField(lua.get(), tableIndex, "shiny_star_x", outLayout.shinyStarRect.x);
    readOptionalNumberField(lua.get(), tableIndex, "shiny_star_y", outLayout.shinyStarRect.y);
    readOptionalNumberField(lua.get(), tableIndex, "shiny_star_w", outLayout.shinyStarRect.w);
    readOptionalNumberField(lua.get(), tableIndex, "shiny_star_h", outLayout.shinyStarRect.h);

    readOptionalNumberField(lua.get(), tableIndex, "eevee_sprite_x", outLayout.eeveeSpriteRect.x);
    readOptionalNumberField(lua.get(), tableIndex, "eevee_sprite_y", outLayout.eeveeSpriteRect.y);
    readOptionalNumberField(lua.get(), tableIndex, "eevee_sprite_w", outLayout.eeveeSpriteRect.w);
    readOptionalNumberField(lua.get(), tableIndex, "eevee_sprite_h", outLayout.eeveeSpriteRect.h);

    readOptionalNumberField(lua.get(), tableIndex, "info_dex_x", outLayout.infoDexX);
    readOptionalNumberField(lua.get(), tableIndex, "info_dex_y", outLayout.infoDexY);
    readOptionalNumberField(lua.get(), tableIndex, "info_name_x", outLayout.infoNameX);
    readOptionalNumberField(lua.get(), tableIndex, "info_name_y", outLayout.infoNameY);
    readOptionalNumberField(lua.get(), tableIndex, "info_type_icon_x", outLayout.infoTypeIconRect.x);
    readOptionalNumberField(lua.get(), tableIndex, "info_type_icon_y", outLayout.infoTypeIconRect.y);
    readOptionalNumberField(lua.get(), tableIndex, "info_type_icon_w", outLayout.infoTypeIconRect.w);
    readOptionalNumberField(lua.get(), tableIndex, "info_type_icon_h", outLayout.infoTypeIconRect.h);
    readOptionalNumberField(lua.get(), tableIndex, "info_ot_x", outLayout.infoOtX);
    readOptionalNumberField(lua.get(), tableIndex, "info_ot_y", outLayout.infoOtY);
    readOptionalNumberField(lua.get(), tableIndex, "info_id_x", outLayout.infoIdX);
    readOptionalNumberField(lua.get(), tableIndex, "info_id_y", outLayout.infoIdY);
    readOptionalNumberField(lua.get(), tableIndex, "info_item_x", outLayout.infoItemX);
    readOptionalNumberField(lua.get(), tableIndex, "info_item_y", outLayout.infoItemY);
    readOptionalNumberField(lua.get(), tableIndex, "info_memo_line1_x", outLayout.infoMemoLine1X);
    readOptionalNumberField(lua.get(), tableIndex, "info_memo_line1_y", outLayout.infoMemoLine1Y);
    readOptionalNumberField(lua.get(), tableIndex, "info_memo_line2_x", outLayout.infoMemoLine2X);
    readOptionalNumberField(lua.get(), tableIndex, "info_memo_line2_y", outLayout.infoMemoLine2Y);

    readOptionalNumberField(lua.get(), tableIndex, "skills_hp_text_x", outLayout.skillsHpTextX);
    readOptionalNumberField(lua.get(), tableIndex, "skills_hp_text_align_width", outLayout.skillsHpTextAlignWidth);
    readOptionalNumberField(lua.get(), tableIndex, "skills_hp_text_y", outLayout.skillsHpTextY);
    readOptionalNumberField(lua.get(), tableIndex, "skills_hp_gauge_x", outLayout.skillsHpGaugeOrigin.x);
    readOptionalNumberField(lua.get(), tableIndex, "skills_hp_gauge_y", outLayout.skillsHpGaugeOrigin.y);
    readOptionalIntegerField(lua.get(), tableIndex, "skills_hp_gauge_segments", outLayout.skillsHpGaugeSegments);

    readOptionalNumberField(lua.get(), tableIndex, "skills_stat_value_x", outLayout.skillsStatValueX);
    readOptionalNumberField(lua.get(), tableIndex, "skills_stat_align_width", outLayout.skillsStatAlignWidth);
    readOptionalNumberField(lua.get(), tableIndex, "skills_attack_y", outLayout.skillsAttackY);
    readOptionalNumberField(lua.get(), tableIndex, "skills_defense_y", outLayout.skillsDefenseY);
    readOptionalNumberField(lua.get(), tableIndex, "skills_spatk_y", outLayout.skillsSpAtkY);
    readOptionalNumberField(lua.get(), tableIndex, "skills_spdef_y", outLayout.skillsSpDefY);
    readOptionalNumberField(lua.get(), tableIndex, "skills_speed_y", outLayout.skillsSpeedY);

    readOptionalNumberField(lua.get(), tableIndex, "skills_exp_value_x", outLayout.skillsExpValueX);
    readOptionalNumberField(lua.get(), tableIndex, "skills_exp_align_width", outLayout.skillsExpAlignWidth);
    readOptionalNumberField(lua.get(), tableIndex, "skills_exp_points_y", outLayout.skillsExpPointsY);
    readOptionalNumberField(lua.get(), tableIndex, "skills_next_level_y", outLayout.skillsNextLevelY);
    readOptionalNumberField(lua.get(), tableIndex, "skills_exp_label_x", outLayout.skillsExpLabelX);
    readOptionalNumberField(lua.get(), tableIndex, "skills_exp_label_y", outLayout.skillsExpLabelY);
    readOptionalNumberField(lua.get(), tableIndex, "skills_next_label_x", outLayout.skillsNextLabelX);
    readOptionalNumberField(lua.get(), tableIndex, "skills_next_label_y", outLayout.skillsNextLabelY);
    readOptionalNumberField(lua.get(), tableIndex, "skills_ability_name_x", outLayout.skillsAbilityNameX);
    readOptionalNumberField(lua.get(), tableIndex, "skills_ability_name_y", outLayout.skillsAbilityNameY);
    readOptionalNumberField(lua.get(), tableIndex, "skills_ability_desc_x", outLayout.skillsAbilityDescX);
    readOptionalNumberField(lua.get(), tableIndex, "skills_ability_desc_y", outLayout.skillsAbilityDescY);
    readOptionalNumberField(lua.get(), tableIndex, "skills_exp_gauge_x", outLayout.skillsExpGaugeOrigin.x);
    readOptionalNumberField(lua.get(), tableIndex, "skills_exp_gauge_y", outLayout.skillsExpGaugeOrigin.y);
    readOptionalIntegerField(lua.get(), tableIndex, "skills_exp_gauge_segments", outLayout.skillsExpGaugeSegments);

    readOptionalNumberField(lua.get(), tableIndex, "moves_name_x", outLayout.movesNameX);
    readOptionalNumberField(lua.get(), tableIndex, "moves_name_base_y", outLayout.movesNameBaseY);
    readOptionalNumberField(lua.get(), tableIndex, "moves_row_step_y", outLayout.movesRowStepY);
    readOptionalNumberField(lua.get(), tableIndex, "moves_pp_y_offset", outLayout.movesPpYOffset);
    readOptionalNumberField(lua.get(), tableIndex, "moves_pp_label_x", outLayout.movesPpLabelX);
    readOptionalNumberField(lua.get(), tableIndex, "moves_current_pp_x", outLayout.movesCurrentPpX);
    readOptionalNumberField(lua.get(), tableIndex, "moves_current_pp_align_width", outLayout.movesCurrentPpAlignWidth);
    readOptionalNumberField(lua.get(), tableIndex, "moves_slash_x", outLayout.movesSlashX);
    readOptionalNumberField(lua.get(), tableIndex, "moves_max_pp_x", outLayout.movesMaxPpX);
    readOptionalNumberField(lua.get(), tableIndex, "moves_max_pp_align_width", outLayout.movesMaxPpAlignWidth);
    readOptionalNumberField(lua.get(), tableIndex, "moves_type_icon_x_offset", outLayout.movesTypeIconXOffset);
    readOptionalNumberField(lua.get(), tableIndex, "moves_type_icon_w", outLayout.movesTypeIconWidth);
    readOptionalNumberField(lua.get(), tableIndex, "moves_type_icon_h", outLayout.movesTypeIconHeight);

    lua_pop(lua.get(), 1);
    return true;
}
