#include "game/ui/PokemonSummaryOverlay.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>

#include <SDL3/SDL_render.h>

#include "engine/TextureManager.h"
#include "game/data/SpeciesRegistry.h"
#include "game/ui/PokemonSummaryLayout.h"
#include "game/ui/PokemonSummaryLayoutProvider.h"
#include "game/ui/PokemonSummaryRenderUtils.h"
#include "game/ui/SummaryContent.h"

namespace {
constexpr const char* kAssetPageInfo = "assets/ui/summary_screen/page_info.png";
constexpr const char* kAssetPageSkills = "assets/ui/summary_screen/page_skills.png";
constexpr const char* kAssetPageMoves = "assets/ui/summary_screen/page_moves.png";
constexpr const char* kAssetBaseInfo = "assets/ui/summary_screen/base_info_info.png";
constexpr const char* kAssetBaseInfoSkills = "assets/ui/summary_screen/base_info_skills.png";
constexpr const char* kAssetMenuInfo = "assets/ui/summary_screen/menu_info.png";
constexpr const char* kAssetPartyPokeball = "assets/ui/summary_screen/ball_icon.png";
constexpr const char* kAssetShinyStar = "assets/ui/summary_screen/shiny_star.png";
constexpr SDL_Color kTextColorLight{255, 255, 255, 255};
constexpr SDL_Color kTextColorLightShadow{98, 98, 98, 255};
constexpr SDL_Color kTextColorDark = summary_render::kTextColorDark;
constexpr SDL_Color kTextColorDarkShadow = summary_render::kTextColorDarkShadow;
constexpr SDL_Color kTextColorMale{164, 197, 246, 255};
constexpr SDL_Color kTextColorMaleShadow{49, 82, 205, 255};
constexpr SDL_Color kTextColorFemale{255, 189, 115, 255};
constexpr SDL_Color kTextColorFemaleShadow{230, 8, 8, 255};

SDL_FRect mapToViewport(
    const SDL_FRect& rect,
    const float scale,
    const float offsetX,
    const float offsetY
) {
    return summary_render::mapToViewport(rect, scale, offsetX, offsetY);
}

float rightAlignOffset(
    TextureManager& textureManager,
    const std::string_view text,
    const float alignWidth,
    const FrlgTextRenderer::Font font
) {
    return summary_render::rightAlignOffset(textureManager, pokemonSummaryLayout(), text, alignWidth, font);
}

void renderDebugText(
    TextureManager& textureManager,
    const std::string_view text,
    const float x,
    const float y,
    const float scale,
    const float offsetX,
    const float offsetY,
    const FrlgTextRenderer::Font font = FrlgTextRenderer::Font::Normal,
    const SDL_Color color = kTextColorDark,
    const SDL_Color shadowColor = kTextColorDarkShadow
) {
    summary_render::renderDebugText(
        textureManager,
        text,
        x,
        y,
        scale,
        offsetX,
        offsetY,
        pokemonSummaryLayout(),
        font,
        color,
        shadowColor
    );
}

void renderDebugTextWindowAligned(
    TextureManager& textureManager,
    const std::string_view text,
    const SDL_FPoint& windowOrigin,
    const float x,
    const float y,
    const float scale,
    const float offsetX,
    const float offsetY,
    const FrlgTextRenderer::Font font = FrlgTextRenderer::Font::Normal,
    const SDL_Color color = kTextColorDark,
    const SDL_Color shadowColor = kTextColorDarkShadow
) {
    summary_render::renderDebugTextWindowAligned(
        textureManager,
        text,
        windowOrigin,
        x,
        y,
        scale,
        offsetX,
        offsetY,
        pokemonSummaryLayout(),
        font,
        color,
        shadowColor
    );
}

} // namespace

void PokemonSummaryOverlay::render(
    TextureManager& textureManager,
    const int viewportWidth,
    const int viewportHeight,
    const std::vector<PartyPokemon>& party
) const {
    SDL_Renderer* renderer = textureManager.renderer();
    if (!open_ || !renderer || viewportWidth <= 0 || viewportHeight <= 0) {
        return;
    }

    const PokemonSummaryLayout& layout = pokemonSummaryLayout();
    const float scaleX = static_cast<float>(viewportWidth) / layout.designWidth;
    const float scaleY = static_cast<float>(viewportHeight) / layout.designHeight;
    const float scale = std::max(0.01f, std::min(scaleX, scaleY));
    const float contentWidth = layout.designWidth * scale;
    const float contentHeight = layout.designHeight * scale;
    const float offsetX = (static_cast<float>(viewportWidth) - contentWidth) * 0.5f;
    const float offsetY = (static_cast<float>(viewportHeight) - contentHeight) * 0.5f;

    const SDL_FRect fullScreenRect{0.0f, 0.0f, static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
    const SDL_FRect contentRect{offsetX, offsetY, contentWidth, contentHeight};

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &fullScreenRect);

    SDL_Texture* infoPageTexture = textureManager.load(kAssetPageInfo);
    SDL_Texture* skillsPageTexture = textureManager.load(kAssetPageSkills);
    SDL_Texture* movesPageTexture = textureManager.load(kAssetPageMoves);
    SDL_Texture* baseInfoTexture = textureManager.load(kAssetBaseInfo);
    SDL_Texture* baseInfoSkillsTexture = textureManager.load(kAssetBaseInfoSkills);
    SDL_Texture* menuInfoTexture = textureManager.load(kAssetMenuInfo);
    SDL_Texture* partyPokeballTexture = textureManager.load(kAssetPartyPokeball);
    SDL_Texture* shinyStarTexture = textureManager.load(kAssetShinyStar);

    SDL_Texture* pageTexture = nullptr;
    SDL_Texture* baseTexture = nullptr;
    switch (page_) {
    case SummaryPage::Info:
        pageTexture = infoPageTexture;
        baseTexture = baseInfoTexture ? baseInfoTexture : baseInfoSkillsTexture;
        break;
    case SummaryPage::Skills:
        pageTexture = skillsPageTexture;
        baseTexture = baseInfoSkillsTexture;
        break;
    case SummaryPage::Moves:
        pageTexture = movesPageTexture;
        baseTexture = baseInfoSkillsTexture;
        break;
    default:
        pageTexture = infoPageTexture;
        baseTexture = baseInfoSkillsTexture;
        break;
    }

    if (baseTexture) {
        SDL_RenderTexture(renderer, baseTexture, nullptr, &contentRect);
    } else {
        SDL_SetRenderDrawColor(renderer, 186, 186, 218, 255);
        SDL_RenderFillRect(renderer, &contentRect);
    }

    if (pageTexture) {
        SDL_RenderTexture(renderer, pageTexture, nullptr, &contentRect);
    }

    const int count = partyCount(party);
    if (count <= 0) {
        renderDebugText(
            textureManager,
            "POKEMON INFO",
            6.0f,
            6.0f,
            scale,
            offsetX,
            offsetY,
            FrlgTextRenderer::Font::Normal,
            kTextColorLight,
            kTextColorLightShadow
        );
        renderDebugText(
            textureManager,
            "No POKEMON.",
            16.0f,
            76.0f,
            scale,
            offsetX,
            offsetY,
            FrlgTextRenderer::Font::Normal,
            kTextColorDark
        );
        return;
    }

    const PartyPokemon& member = party[static_cast<std::size_t>(selectedIndex_)];
    const SpeciesDefinition* speciesData = summary_content::speciesDefinitionFor(member);
    const DisplayStats stats = makeDisplayStats(member);
    const char* speciesName = (speciesData && !speciesData->name.empty()) ? speciesData->name.c_str() : "POKEMON";

    const int pageIndex = static_cast<int>(page_);
    renderDebugTextWindowAligned(
        textureManager,
        summary_content::pageTitleFor(pageIndex),
        layout.windowPageName,
        layout.headerTitleX,
        layout.headerTitleY,
        scale,
        offsetX,
        offsetY,
        FrlgTextRenderer::Font::Normal,
        kTextColorLight,
        kTextColorLightShadow
    );
    const std::string controlsText = summary_content::controlsTextFor(pageIndex);
    if (!controlsText.empty()) {
        renderDebugTextWindowAligned(
            textureManager,
            controlsText,
            layout.windowControls,
            rightAlignOffset(textureManager, controlsText, layout.controlsAlignWidth, FrlgTextRenderer::Font::Small),
            layout.controlsY,
            scale,
            offsetX,
            offsetY,
            FrlgTextRenderer::Font::Small,
            kTextColorLight,
            kTextColorLightShadow
        );
    }

    std::ostringstream levelText;
    levelText << "Lv" << member.level;
    renderDebugTextWindowAligned(
        textureManager,
        levelText.str(),
        layout.windowLevelNick,
        layout.levelTextX,
        layout.levelSpeciesY,
        scale,
        offsetX,
        offsetY,
        FrlgTextRenderer::Font::Normal,
        kTextColorLight,
        kTextColorLightShadow
    );
    renderDebugTextWindowAligned(
        textureManager,
        speciesName,
        layout.windowLevelNick,
        layout.speciesTextX,
        layout.levelSpeciesY,
        scale,
        offsetX,
        offsetY,
        FrlgTextRenderer::Font::Normal,
        kTextColorLight,
        kTextColorLightShadow
    );
    const bool isFemale = false;
    renderDebugTextWindowAligned(
        textureManager,
        isFemale ? "&" : "^",
        layout.windowLevelNick,
        layout.genderSymbolX,
        layout.levelSpeciesY,
        scale,
        offsetX,
        offsetY,
        FrlgTextRenderer::Font::Normal,
        isFemale ? kTextColorFemale : kTextColorMale,
        isFemale ? kTextColorFemaleShadow : kTextColorMaleShadow
    );

    if (speciesData && !speciesData->frontSpritePath.empty()) {
        if (SDL_Texture* eeveeFrontTexture = textureManager.load(speciesData->frontSpritePath.c_str())) {
            const SDL_FRect src{0.0f, 0.0f, 64.0f, 64.0f};
            const SDL_FRect dst = mapToViewport(layout.eeveeSpriteRect, scale, offsetX, offsetY);
            SDL_RenderTexture(renderer, eeveeFrontTexture, &src, &dst);
        }
    }

    if (partyPokeballTexture) {
        const SDL_FRect src{0.0f, 0.0f, 16.0f, 16.0f};
        const SDL_FRect dst = mapToViewport(layout.monBallIconRect, scale, offsetX, offsetY);
        SDL_RenderTexture(renderer, partyPokeballTexture, &src, &dst);
    }

    if (member.isPartner && shinyStarTexture) {
        const SDL_FRect src{0.0f, 0.0f, 8.0f, 8.0f};
        const SDL_FRect dst = mapToViewport(layout.shinyStarRect, scale, offsetX, offsetY);
        SDL_RenderTexture(renderer, shinyStarTexture, &src, &dst);
    }

    switch (page_) {
    case SummaryPage::Info:
        renderInfoPage(
            textureManager,
            renderer,
            menuInfoTexture,
            member,
            speciesName,
            speciesData,
            scale,
            offsetX,
            offsetY,
            layout
        );
        break;
    case SummaryPage::Skills:
        renderSkillsPage(textureManager, renderer, member, stats, scale, offsetX, offsetY, layout);
        break;
    case SummaryPage::Moves:
        renderMovesPage(textureManager, renderer, menuInfoTexture, member, stats, scale, offsetX, offsetY, layout);
        break;
    default:
        break;
    }
}
