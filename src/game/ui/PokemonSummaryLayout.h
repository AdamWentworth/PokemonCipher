#pragma once

#include <string>

#include <SDL3/SDL_rect.h>

struct PokemonSummaryLayout {
    float designWidth = 240.0f;
    float designHeight = 160.0f;
    float textBaselineYOffset = 2.0f;
    float textScaleMultiplier = 1.5f;
    bool drawGaugeBackground = true;

    SDL_FPoint windowPageName{0.0f, 0.0f};
    SDL_FPoint windowControls{152.0f, 0.0f};
    SDL_FPoint windowLevelNick{0.0f, 17.0f};
    SDL_FPoint windowInfoRightPane{120.0f, 17.0f};
    SDL_FPoint windowInfoTrainerMemo{8.0f, 113.0f};
    SDL_FPoint windowSkillsRightPane{160.0f, 17.0f};
    SDL_FPoint windowSkillsExpText{48.0f, 97.0f};
    SDL_FPoint windowSkillsAbility{8.0f, 129.0f};
    SDL_FPoint windowMovesRightPane{160.0f, 17.0f};
    SDL_FPoint windowMovesTypePane{120.0f, 17.0f};

    float headerTitleX = 4.0f;
    float headerTitleY = 1.0f;
    float controlsAlignWidth = 87.0f;
    float controlsY = 0.0f;

    float levelTextX = 4.0f;
    float speciesTextX = 40.0f;
    float genderSymbolX = 105.0f;
    float levelSpeciesY = 2.0f;
    SDL_FRect monBallIconRect{98.0f, 80.0f, 16.0f, 16.0f};
    SDL_FRect shinyStarRect{102.0f, 36.0f, 8.0f, 8.0f};

    SDL_FRect eeveeSpriteRect{28.0f, 33.0f, 64.0f, 64.0f};

    float infoDexX = 47.0f;
    float infoDexY = 5.0f;
    float infoNameX = 47.0f;
    float infoNameY = 19.0f;
    SDL_FRect infoTypeIconRect{167.0f, 51.0f, 32.0f, 12.0f};
    float infoOtX = 47.0f;
    float infoOtY = 49.0f;
    float infoIdX = 47.0f;
    float infoIdY = 64.0f;
    float infoItemX = 47.0f;
    float infoItemY = 79.0f;
    float infoMemoLine1X = 0.0f;
    float infoMemoLine1Y = 3.0f;
    float infoMemoLine2X = 0.0f;
    float infoMemoLine2Y = 16.0f;

    float skillsHpTextX = 14.0f;
    float skillsHpTextAlignWidth = 63.0f;
    float skillsHpTextY = 4.0f;
    SDL_FPoint skillsHpGaugeOrigin{166.0f, 32.0f};
    int skillsHpGaugeSegments = 6;

    float skillsStatValueX = 50.0f;
    float skillsStatAlignWidth = 27.0f;
    float skillsAttackY = 22.0f;
    float skillsDefenseY = 35.0f;
    float skillsSpAtkY = 48.0f;
    float skillsSpDefY = 61.0f;
    float skillsSpeedY = 74.0f;

    float skillsExpValueX = 15.0f;
    float skillsExpAlignWidth = 63.0f;
    float skillsExpPointsY = 87.0f;
    float skillsNextLevelY = 100.0f;
    float skillsExpLabelX = 26.0f;
    float skillsExpLabelY = 7.0f;
    float skillsNextLabelX = 26.0f;
    float skillsNextLabelY = 20.0f;
    float skillsAbilityNameX = 66.0f;
    float skillsAbilityNameY = 1.0f;
    float skillsAbilityDescX = 2.0f;
    float skillsAbilityDescY = 15.0f;
    SDL_FPoint skillsExpGaugeOrigin{150.0f, 128.0f};
    int skillsExpGaugeSegments = 8;

    float movesNameX = 3.0f;
    float movesNameBaseY = 5.0f;
    float movesRowStepY = 28.0f;
    float movesPpYOffset = 11.0f;
    float movesPpLabelX = 36.0f;
    float movesCurrentPpX = 46.0f;
    float movesCurrentPpAlignWidth = 16.0f;
    float movesSlashX = 58.0f;
    float movesMaxPpX = 64.0f;
    float movesMaxPpAlignWidth = 16.0f;
    float movesTypeIconXOffset = 3.0f;
    float movesTypeIconWidth = 32.0f;
    float movesTypeIconHeight = 12.0f;
};

PokemonSummaryLayout makeDefaultPokemonSummaryLayout();
bool loadPokemonSummaryLayoutFromLuaFile(
    const std::string& scriptPath,
    PokemonSummaryLayout& outLayout,
    std::string& errorOut
);
