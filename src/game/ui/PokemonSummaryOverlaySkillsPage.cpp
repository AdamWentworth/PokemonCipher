#include "game/ui/PokemonSummaryOverlay.h"

#include <sstream>

#include <SDL3/SDL_render.h>

#include "engine/TextureManager.h"
#include "game/ui/PokemonSummaryLayout.h"
#include "game/ui/PokemonSummaryRenderUtils.h"
#include "game/ui/SummaryContent.h"

namespace {
constexpr const char* kAssetHpBar = "assets/ui/summary_screen/hp_bar.png";
constexpr const char* kAssetExpBar = "assets/ui/summary_screen/exp_bar.png";
}

void PokemonSummaryOverlay::renderSkillsPage(
    TextureManager& textureManager,
    SDL_Renderer* renderer,
    const PartyPokemon& member,
    const DisplayStats& stats,
    const float scale,
    const float offsetX,
    const float offsetY,
    const PokemonSummaryLayout& layout
) const {
    std::ostringstream hpText;
    hpText << stats.hp << "/" << stats.maxHp;
    summary_render::renderDebugTextWindowRightAligned(
        textureManager,
        hpText.str(),
        layout.windowSkillsRightPane,
        layout.skillsHpTextX,
        layout.skillsHpTextAlignWidth,
        layout.skillsHpTextY,
        scale,
        offsetX,
        offsetY,
        layout
    );
    summary_render::renderSegmentedGauge(
        renderer,
        textureManager.load(kAssetHpBar),
        layout.skillsHpGaugeOrigin,
        layout.skillsHpGaugeSegments,
        stats.hpRatio,
        scale,
        offsetX,
        offsetY,
        layout
    );

    std::ostringstream atkText;
    atkText << stats.attack;
    summary_render::renderDebugTextWindowRightAligned(
        textureManager,
        atkText.str(),
        layout.windowSkillsRightPane,
        layout.skillsStatValueX,
        layout.skillsStatAlignWidth,
        layout.skillsAttackY,
        scale,
        offsetX,
        offsetY,
        layout
    );
    std::ostringstream defText;
    defText << stats.defense;
    summary_render::renderDebugTextWindowRightAligned(
        textureManager,
        defText.str(),
        layout.windowSkillsRightPane,
        layout.skillsStatValueX,
        layout.skillsStatAlignWidth,
        layout.skillsDefenseY,
        scale,
        offsetX,
        offsetY,
        layout
    );
    std::ostringstream spaText;
    spaText << stats.spAttack;
    summary_render::renderDebugTextWindowRightAligned(
        textureManager,
        spaText.str(),
        layout.windowSkillsRightPane,
        layout.skillsStatValueX,
        layout.skillsStatAlignWidth,
        layout.skillsSpAtkY,
        scale,
        offsetX,
        offsetY,
        layout
    );
    std::ostringstream spdText;
    spdText << stats.spDefense;
    summary_render::renderDebugTextWindowRightAligned(
        textureManager,
        spdText.str(),
        layout.windowSkillsRightPane,
        layout.skillsStatValueX,
        layout.skillsStatAlignWidth,
        layout.skillsSpDefY,
        scale,
        offsetX,
        offsetY,
        layout
    );
    std::ostringstream speText;
    speText << stats.speed;
    summary_render::renderDebugTextWindowRightAligned(
        textureManager,
        speText.str(),
        layout.windowSkillsRightPane,
        layout.skillsStatValueX,
        layout.skillsStatAlignWidth,
        layout.skillsSpeedY,
        scale,
        offsetX,
        offsetY,
        layout
    );

    std::ostringstream expText;
    expText << stats.expPoints;
    summary_render::renderDebugTextWindowRightAligned(
        textureManager,
        expText.str(),
        layout.windowSkillsRightPane,
        layout.skillsExpValueX,
        layout.skillsExpAlignWidth,
        layout.skillsExpPointsY,
        scale,
        offsetX,
        offsetY,
        layout
    );
    std::ostringstream nextText;
    nextText << stats.expToNextLevel;
    summary_render::renderDebugTextWindowRightAligned(
        textureManager,
        nextText.str(),
        layout.windowSkillsRightPane,
        layout.skillsExpValueX,
        layout.skillsExpAlignWidth,
        layout.skillsNextLevelY,
        scale,
        offsetX,
        offsetY,
        layout
    );

    summary_render::renderDebugTextWindowAligned(
        textureManager,
        "EXP. POINTS",
        layout.windowSkillsExpText,
        layout.skillsExpLabelX,
        layout.skillsExpLabelY,
        scale,
        offsetX,
        offsetY,
        layout
    );
    summary_render::renderDebugTextWindowAligned(
        textureManager,
        "NEXT LV.",
        layout.windowSkillsExpText,
        layout.skillsNextLabelX,
        layout.skillsNextLabelY,
        scale,
        offsetX,
        offsetY,
        layout
    );

    summary_render::renderDebugTextWindowAligned(
        textureManager,
        summary_content::abilityNameFor(member),
        layout.windowSkillsAbility,
        layout.skillsAbilityNameX,
        layout.skillsAbilityNameY,
        scale,
        offsetX,
        offsetY,
        layout
    );
    summary_render::renderDebugTextWindowAligned(
        textureManager,
        summary_content::abilityDescriptionFor(member),
        layout.windowSkillsAbility,
        layout.skillsAbilityDescX,
        layout.skillsAbilityDescY,
        scale,
        offsetX,
        offsetY,
        layout
    );

    summary_render::renderSegmentedGauge(
        renderer,
        textureManager.load(kAssetExpBar),
        layout.skillsExpGaugeOrigin,
        layout.skillsExpGaugeSegments,
        stats.expRatio,
        scale,
        offsetX,
        offsetY,
        layout
    );
}

