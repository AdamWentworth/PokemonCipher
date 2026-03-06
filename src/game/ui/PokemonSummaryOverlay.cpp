#include "game/ui/PokemonSummaryOverlay.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include <SDL3/SDL_render.h>

#include "engine/TextureManager.h"
#include "game/data/SpeciesRegistry.h"
#include "game/ui/PokemonSummaryLayout.h"
#include "game/ui/FrlgTextRenderer.h"

namespace {
constexpr const char* kAssetPageInfo = "assets/ui/summary_screen/page_info.png";
constexpr const char* kAssetPageSkills = "assets/ui/summary_screen/page_skills.png";
constexpr const char* kAssetPageMoves = "assets/ui/summary_screen/page_moves.png";
constexpr const char* kAssetBaseInfo = "assets/ui/summary_screen/base_info_info.png";
constexpr const char* kAssetBaseInfoSkills = "assets/ui/summary_screen/base_info_skills.png";
constexpr const char* kAssetHpBar = "assets/ui/summary_screen/hp_bar.png";
constexpr const char* kAssetExpBar = "assets/ui/summary_screen/exp_bar.png";
constexpr const char* kAssetMenuInfo = "assets/ui/summary_screen/menu_info.png";
constexpr const char* kAssetPartyPokeball = "assets/ui/summary_screen/ball_icon.png";
constexpr const char* kAssetShinyStar = "assets/ui/summary_screen/shiny_star.png";
constexpr SDL_Color kTextColorLight{255, 255, 255, 255};
constexpr SDL_Color kTextColorLightShadow{98, 98, 98, 255};
constexpr SDL_Color kTextColorDark{65, 65, 65, 255};
constexpr SDL_Color kTextColorDarkShadow{222, 222, 197, 255};
constexpr SDL_Color kTextColorMale{164, 197, 246, 255};
constexpr SDL_Color kTextColorMaleShadow{49, 82, 205, 255};
constexpr SDL_Color kTextColorFemale{255, 189, 115, 255};
constexpr SDL_Color kTextColorFemaleShadow{230, 8, 8, 255};
constexpr SDL_Color kMoveTextColorNormal{32, 32, 32, 255};
constexpr SDL_Color kMoveTextColorNormalShadow{222, 222, 222, 255};
constexpr SDL_Color kMoveTextColorLow{238, 222, 0, 255};
constexpr SDL_Color kMoveTextColorLowShadow{255, 246, 139, 255};
constexpr SDL_Color kMoveTextColorVeryLow{255, 148, 0, 255};
constexpr SDL_Color kMoveTextColorVeryLowShadow{255, 238, 115, 255};
constexpr SDL_Color kMoveTextColorEmpty{238, 0, 0, 255};
constexpr SDL_Color kMoveTextColorEmptyShadow{246, 222, 156, 255};

constexpr int kTypeNormal = 0;
constexpr int kTypeFighting = 1;
constexpr int kTypeFlying = 2;
constexpr int kTypePoison = 3;
constexpr int kTypeGround = 4;
constexpr int kTypeRock = 5;
constexpr int kTypeBug = 6;
constexpr int kTypeGhost = 7;
constexpr int kTypeSteel = 8;
constexpr int kTypeMystery = 9;
constexpr int kTypeFire = 10;
constexpr int kTypeWater = 11;
constexpr int kTypeGrass = 12;
constexpr int kTypeElectric = 13;
constexpr int kTypePsychic = 14;
constexpr int kTypeIce = 15;
constexpr int kTypeDragon = 16;
constexpr int kTypeDark = 17;

const PokemonSummaryLayout& summaryLayout();

SDL_FRect mapToViewport(
    const SDL_FRect& rect,
    const float scale,
    const float offsetX,
    const float offsetY
) {
    return SDL_FRect{
        offsetX + (rect.x * scale),
        offsetY + (rect.y * scale),
        rect.w * scale,
        rect.h * scale,
    };
}

float mapX(const float x, const float scale, const float offsetX) {
    return offsetX + (x * scale);
}

float mapY(const float y, const float scale, const float offsetY) {
    return offsetY + ((y + summaryLayout().textBaselineYOffset) * scale);
}

FrlgTextRenderer& summaryTextRenderer() {
    static FrlgTextRenderer renderer;
    return renderer;
}

const PokemonSummaryLayout& summaryLayout() {
    static const PokemonSummaryLayout layout = []() {
        PokemonSummaryLayout loaded = makeDefaultPokemonSummaryLayout();
        std::string error;
        if (!loadPokemonSummaryLayoutFromLuaFile("assets/config/ui/pokemon_summary_layout.lua", loaded, error) &&
            !error.empty()) {
            std::cout << "Summary layout config load failed; using defaults: " << error << '\n';
        }
        return loaded;
    }();
    return layout;
}

SDL_FRect iconRectFromTileOffset(const int tileOffset, const float width, const float height) {
    constexpr int tilesPerRow = 16;
    const int tileX = tileOffset % tilesPerRow;
    const int tileY = tileOffset / tilesPerRow;
    return SDL_FRect{
        static_cast<float>(tileX * 8),
        static_cast<float>(tileY * 8),
        width,
        height
    };
}

bool tryGetTypeIconSourceRect(const int typeId, SDL_FRect& outRect) {
    switch (typeId) {
    case kTypeNormal:
        outRect = iconRectFromTileOffset(0x20, 32.0f, 12.0f);
        return true;
    case kTypeFighting:
        outRect = iconRectFromTileOffset(0x64, 32.0f, 12.0f);
        return true;
    case kTypeFlying:
        outRect = iconRectFromTileOffset(0x60, 32.0f, 12.0f);
        return true;
    case kTypePoison:
        outRect = iconRectFromTileOffset(0x80, 32.0f, 12.0f);
        return true;
    case kTypeGround:
        outRect = iconRectFromTileOffset(0x48, 32.0f, 12.0f);
        return true;
    case kTypeRock:
        outRect = iconRectFromTileOffset(0x44, 32.0f, 12.0f);
        return true;
    case kTypeBug:
        outRect = iconRectFromTileOffset(0x6C, 32.0f, 12.0f);
        return true;
    case kTypeGhost:
        outRect = iconRectFromTileOffset(0x68, 32.0f, 12.0f);
        return true;
    case kTypeSteel:
        outRect = iconRectFromTileOffset(0x88, 32.0f, 12.0f);
        return true;
    case kTypeMystery:
        outRect = iconRectFromTileOffset(0xA4, 32.0f, 12.0f);
        return true;
    case kTypeFire:
        outRect = iconRectFromTileOffset(0x24, 32.0f, 12.0f);
        return true;
    case kTypeWater:
        outRect = iconRectFromTileOffset(0x28, 32.0f, 12.0f);
        return true;
    case kTypeGrass:
        outRect = iconRectFromTileOffset(0x2C, 32.0f, 12.0f);
        return true;
    case kTypeElectric:
        outRect = iconRectFromTileOffset(0x40, 32.0f, 12.0f);
        return true;
    case kTypePsychic:
        outRect = iconRectFromTileOffset(0x84, 32.0f, 12.0f);
        return true;
    case kTypeIce:
        outRect = iconRectFromTileOffset(0x4C, 32.0f, 12.0f);
        return true;
    case kTypeDragon:
        outRect = iconRectFromTileOffset(0xA0, 32.0f, 12.0f);
        return true;
    case kTypeDark:
        outRect = iconRectFromTileOffset(0x8C, 32.0f, 12.0f);
        return true;
    default:
        return false;
    }
}

void renderTypeIcon(
    SDL_Renderer* renderer,
    SDL_Texture* menuInfoTexture,
    const int typeId,
    const SDL_FRect& designDst,
    const float scale,
    const float offsetX,
    const float offsetY
) {
    if (!renderer || !menuInfoTexture) {
        return;
    }

    SDL_FRect srcRect{};
    if (!tryGetTypeIconSourceRect(typeId, srcRect)) {
        return;
    }

    const SDL_FRect dstRect = mapToViewport(designDst, scale, offsetX, offsetY);
    SDL_RenderTexture(renderer, menuInfoTexture, &srcRect, &dstRect);
}

float rightAlignOffset(
    TextureManager& textureManager,
    const std::string_view text,
    const float alignWidth,
    const FrlgTextRenderer::Font font
) {
    const float textWidth =
        summaryTextRenderer().measureText(textureManager, text, font) * summaryLayout().textScaleMultiplier;
    return std::max(0.0f, alignWidth - textWidth);
}

void renderDebugText(
    TextureManager& textureManager,
    const std::string& text,
    const float x,
    const float y,
    const float scale,
    const float offsetX,
    const float offsetY,
    const FrlgTextRenderer::Font font = FrlgTextRenderer::Font::Normal,
    const SDL_Color color = kTextColorDark,
    const SDL_Color shadowColor = kTextColorDarkShadow
) {
    const float textScale = scale * summaryLayout().textScaleMultiplier;
    summaryTextRenderer().drawText(
        textureManager,
        text,
        mapX(x, scale, offsetX),
        mapY(y, scale, offsetY),
        textScale,
        font,
        color,
        shadowColor
    );
}

void renderDebugText(
    TextureManager& textureManager,
    const char* text,
    const float x,
    const float y,
    const float scale,
    const float offsetX,
    const float offsetY,
    const FrlgTextRenderer::Font font = FrlgTextRenderer::Font::Normal,
    const SDL_Color color = kTextColorDark,
    const SDL_Color shadowColor = kTextColorDarkShadow
) {
    const float textScale = scale * summaryLayout().textScaleMultiplier;
    summaryTextRenderer().drawText(
        textureManager,
        text,
        mapX(x, scale, offsetX),
        mapY(y, scale, offsetY),
        textScale,
        font,
        color,
        shadowColor
    );
}

void renderDebugTextWindowAligned(
    TextureManager& textureManager,
    const std::string& text,
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
    renderDebugText(
        textureManager,
        text,
        windowOrigin.x + x,
        windowOrigin.y + y,
        scale,
        offsetX,
        offsetY,
        font,
        color,
        shadowColor
    );
}

void renderDebugTextWindowAligned(
    TextureManager& textureManager,
    const char* text,
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
    renderDebugText(
        textureManager,
        text,
        windowOrigin.x + x,
        windowOrigin.y + y,
        scale,
        offsetX,
        offsetY,
        font,
        color,
        shadowColor
    );
}

void renderDebugTextWindowRightAligned(
    TextureManager& textureManager,
    const std::string& text,
    const SDL_FPoint& windowOrigin,
    const float x,
    const float alignWidth,
    const float y,
    const float scale,
    const float offsetX,
    const float offsetY,
    const FrlgTextRenderer::Font font = FrlgTextRenderer::Font::Normal,
    const SDL_Color color = kTextColorDark,
    const SDL_Color shadowColor = kTextColorDarkShadow
) {
    renderDebugTextWindowAligned(
        textureManager,
        text,
        windowOrigin,
        x + rightAlignOffset(textureManager, text, alignWidth, font),
        y,
        scale,
        offsetX,
        offsetY,
        font,
        color,
        shadowColor
    );
}

void renderSegmentedGauge(
    SDL_Renderer* renderer,
    SDL_Texture* gaugeTexture,
    const SDL_FPoint origin,
    const int middleSegments,
    float ratio,
    const float scale,
    const float offsetX,
    const float offsetY
) {
    if (!renderer || middleSegments <= 0) {
        return;
    }

    const PokemonSummaryLayout& layout = summaryLayout();
    ratio = std::clamp(ratio, 0.0f, 1.0f);
    if (!gaugeTexture) {
        SDL_FRect fallback = mapToViewport(
            SDL_FRect{
                origin.x,
                origin.y,
                static_cast<float>((middleSegments + 3) * 8),
                8.0f
            },
            scale,
            offsetX,
            offsetY
        );
        if (layout.drawGaugeBackground) {
            SDL_SetRenderDrawColor(renderer, 64, 74, 96, 255);
            SDL_RenderFillRect(renderer, &fallback);
        }
        SDL_FRect fill = fallback;
        fill.w *= ratio;
        SDL_SetRenderDrawColor(renderer, 92, 200, 108, 255);
        SDL_RenderFillRect(renderer, &fill);
        return;
    }

    const auto drawFrameAt = [&](const int frameIndex, const int segmentIndex) {
        const SDL_FRect src{
            static_cast<float>(frameIndex * 8),
            0.0f,
            8.0f,
            8.0f
        };
        const SDL_FRect dst = mapToViewport(
            SDL_FRect{
                origin.x + static_cast<float>(segmentIndex * 8),
                origin.y,
                8.0f,
                8.0f
            },
            scale,
            offsetX,
            offsetY
        );
        SDL_RenderTexture(renderer, gaugeTexture, &src, &dst);
    };

    // Frames 9,10,11 are fixed left-left, left-right, right endcap in FireRed gauge art.
    // Keep caps visible even when we hide empty middle segments.
    drawFrameAt(9, 0);
    drawFrameAt(10, 1);

    for (int i = 0; i < middleSegments; ++i) {
        const float segmentStart = static_cast<float>(i) / static_cast<float>(middleSegments);
        const float segmentEnd = static_cast<float>(i + 1) / static_cast<float>(middleSegments);

        int frame = 0;
        if (ratio >= segmentEnd) {
            frame = 8;
        } else if (ratio > segmentStart) {
            const float localRatio = (ratio - segmentStart) / (segmentEnd - segmentStart);
            frame = std::clamp(static_cast<int>(std::ceil(localRatio * 8.0f)), 1, 7);
        }

        if (layout.drawGaugeBackground || frame > 0) {
            drawFrameAt(frame, i + 2);
        }
    }

    drawFrameAt(11, middleSegments + 2);
}

const char* pageTitleFor(const int pageIndex) {
    switch (pageIndex) {
    case 0:
        return "POKEMON INFO";
    case 1:
        return "POKEMON SKILLS";
    case 2:
        return "KNOWN MOVES";
    default:
        return "POKEMON INFO";
    }
}

const char* controlsTextFor(const int pageIndex) {
    (void)pageIndex;
    return "";
}

const char* itemTextFor(const PartyPokemon& member) {
    if (member.isPartner) {
        return "NONE";
    }
    return "NONE";
}

struct MoveDisplay {
    const char* name = "-------";
    int pp = 0;
    int maxPp = 0;
    int typeId = kTypeMystery;
};

struct SpeciesTypeDisplay {
    int primary = kTypeMystery;
    int secondary = kTypeMystery;
};

const SpeciesDefinition* speciesDefinitionFor(const PartyPokemon& member) {
    return getSpeciesRegistry().find(member.speciesId);
}

const char* abilityNameFor(const PartyPokemon& member) {
    if (const SpeciesDefinition* species = speciesDefinitionFor(member);
        species != nullptr && !species->abilityName.empty()) {
        return species->abilityName.c_str();
    }
    return "UNKNOWN";
}

const char* abilityDescriptionFor(const PartyPokemon& member) {
    if (const SpeciesDefinition* species = speciesDefinitionFor(member);
        species != nullptr && !species->abilityDescription.empty()) {
        return species->abilityDescription.c_str();
    }
    return "No ability data.";
}

SpeciesTypeDisplay typeIdsForSpecies(const PartyPokemon& member) {
    if (const SpeciesDefinition* species = speciesDefinitionFor(member)) {
        return SpeciesTypeDisplay{species->primaryTypeId, species->secondaryTypeId};
    }
    return SpeciesTypeDisplay{kTypeMystery, kTypeMystery};
}

int movePpColorIndex(const int pp, const int maxPp) {
    if (maxPp <= 0 || pp >= maxPp) {
        return 0;
    }

    if (pp <= 0) {
        return 3;
    }

    if (maxPp == 3) {
        if (pp == 2) {
            return 2;
        }
        if (pp == 1) {
            return 1;
        }
    } else if (maxPp == 2) {
        if (pp == 1) {
            return 1;
        }
    } else {
        if (pp <= (maxPp / 4)) {
            return 2;
        }
        if (pp <= (maxPp / 2)) {
            return 1;
        }
    }

    return 0;
}

SDL_Color movePpColorFromIndex(const int colorIndex) {
    switch (colorIndex) {
    case 1:
        return kMoveTextColorLow;
    case 2:
        return kMoveTextColorVeryLow;
    case 3:
        return kMoveTextColorEmpty;
    default:
        return kMoveTextColorNormal;
    }
}

SDL_Color movePpShadowColorFromIndex(const int colorIndex) {
    switch (colorIndex) {
    case 1:
        return kMoveTextColorLowShadow;
    case 2:
        return kMoveTextColorVeryLowShadow;
    case 3:
        return kMoveTextColorEmptyShadow;
    default:
        return kMoveTextColorNormalShadow;
    }
}

std::array<MoveDisplay, 4> movesFor(const PartyPokemon& member) {
    std::array<MoveDisplay, 4> result{{
        {"-------", 0, 0, kTypeMystery},
        {"-------", 0, 0, kTypeMystery},
        {"-------", 0, 0, kTypeMystery},
        {"-------", 0, 0, kTypeMystery},
    }};

    const SpeciesDefinition* species = speciesDefinitionFor(member);
    if (!species) {
        return result;
    }

    for (std::size_t i = 0; i < result.size(); ++i) {
        const SpeciesMoveDefinition& move = species->moves[i];
        result[i].name = move.name.empty() ? "-------" : move.name.c_str();
        result[i].pp = std::max(0, move.pp);
        result[i].maxPp = std::max(0, move.maxPp);
        result[i].typeId = move.typeId;
    }
    return result;
}
} // namespace

void PokemonSummaryOverlay::open(const std::vector<PartyPokemon>& party, const int selectedIndex) {
    const int count = partyCount(party);
    if (count <= 0) {
        open_ = false;
        selectedIndex_ = 0;
        return;
    }

    open_ = true;
    selectedIndex_ = std::clamp(selectedIndex, 0, count - 1);
    page_ = SummaryPage::Info;
}

void PokemonSummaryOverlay::close() {
    open_ = false;
}

PokemonSummaryAction PokemonSummaryOverlay::handleKey(const SDL_Keycode key, const std::vector<PartyPokemon>& party) {
    if (!open_) {
        return PokemonSummaryAction::None;
    }

    if (key == SDLK_ESCAPE || key == SDLK_X || key == SDLK_BACKSPACE) {
        close();
        return PokemonSummaryAction::Closed;
    }

    const int count = partyCount(party);
    if (count <= 0) {
        selectedIndex_ = 0;
        return PokemonSummaryAction::None;
    }

    if (key == SDLK_LEFT || key == SDLK_A || key == SDLK_RIGHT || key == SDLK_D) {
        flipPage(key);
        return PokemonSummaryAction::None;
    }

    if (key == SDLK_UP || key == SDLK_W || key == SDLK_DOWN || key == SDLK_S) {
        moveMonSelection(key, count);
    }

    return PokemonSummaryAction::None;
}

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

    const PokemonSummaryLayout& layout = summaryLayout();
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
    const SpeciesDefinition* speciesData = speciesDefinitionFor(member);
    const DisplayStats stats = makeDisplayStats(member);
    const char* speciesName = (speciesData && !speciesData->name.empty()) ? speciesData->name.c_str() : "POKEMON";

    const int pageIndex = static_cast<int>(page_);
    renderDebugTextWindowAligned(
        textureManager,
        pageTitleFor(pageIndex),
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
    const std::string controlsText = controlsTextFor(pageIndex);
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
            SDL_FRect src{0.0f, 0.0f, 64.0f, 64.0f};
            SDL_FRect dst = mapToViewport(layout.eeveeSpriteRect, scale, offsetX, offsetY);
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

    if (page_ == SummaryPage::Info) {
        std::ostringstream dexText;
        dexText << member.speciesId;
        renderDebugTextWindowAligned(
            textureManager,
            dexText.str(),
            layout.windowInfoRightPane,
            layout.infoDexX,
            layout.infoDexY,
            scale,
            offsetX,
            offsetY
        );
        renderDebugTextWindowAligned(
            textureManager,
            speciesName,
            layout.windowInfoRightPane,
            layout.infoNameX,
            layout.infoNameY,
            scale,
            offsetX,
            offsetY
        );

        const SpeciesTypeDisplay speciesTypes = typeIdsForSpecies(member);
        renderTypeIcon(
            renderer,
            menuInfoTexture,
            speciesTypes.primary,
            layout.infoTypeIconRect,
            scale,
            offsetX,
            offsetY
        );
        if (speciesTypes.secondary != speciesTypes.primary) {
            SDL_FRect secondaryTypeRect = layout.infoTypeIconRect;
            secondaryTypeRect.x += 36.0f; // FireRed draws dual types at x=47 and x=83 in the right pane.
            renderTypeIcon(
                renderer,
                menuInfoTexture,
                speciesTypes.secondary,
                secondaryTypeRect,
                scale,
                offsetX,
                offsetY
            );
        }

        renderDebugTextWindowAligned(
            textureManager,
            "ADAM",
            layout.windowInfoRightPane,
            layout.infoOtX,
            layout.infoOtY,
            scale,
            offsetX,
            offsetY
        );
        renderDebugTextWindowAligned(
            textureManager,
            "00001",
            layout.windowInfoRightPane,
            layout.infoIdX,
            layout.infoIdY,
            scale,
            offsetX,
            offsetY
        );
        renderDebugTextWindowAligned(
            textureManager,
            itemTextFor(member),
            layout.windowInfoRightPane,
            layout.infoItemX,
            layout.infoItemY,
            scale,
            offsetX,
            offsetY
        );

        renderDebugTextWindowAligned(
            textureManager,
            "Relaxed nature.",
            layout.windowInfoTrainerMemo,
            layout.infoMemoLine1X,
            layout.infoMemoLine1Y,
            scale,
            offsetX,
            offsetY
        );
        renderDebugTextWindowAligned(
            textureManager,
            "Met in PALLET TOWN at Lv5.",
            layout.windowInfoTrainerMemo,
            layout.infoMemoLine2X,
            layout.infoMemoLine2Y,
            scale,
            offsetX,
            offsetY
        );
    } else if (page_ == SummaryPage::Skills) {
        std::ostringstream hpText;
        hpText << stats.hp << "/" << stats.maxHp;
        renderDebugTextWindowRightAligned(
            textureManager,
            hpText.str(),
            layout.windowSkillsRightPane,
            layout.skillsHpTextX,
            layout.skillsHpTextAlignWidth,
            layout.skillsHpTextY,
            scale,
            offsetX,
            offsetY
        );
        renderSegmentedGauge(
            renderer,
            textureManager.load(kAssetHpBar),
            layout.skillsHpGaugeOrigin,
            layout.skillsHpGaugeSegments,
            stats.hpRatio,
            scale,
            offsetX,
            offsetY
        );

        std::ostringstream atkText;
        atkText << stats.attack;
        renderDebugTextWindowRightAligned(
            textureManager,
            atkText.str(),
            layout.windowSkillsRightPane,
            layout.skillsStatValueX,
            layout.skillsStatAlignWidth,
            layout.skillsAttackY,
            scale,
            offsetX,
            offsetY
        );
        std::ostringstream defText;
        defText << stats.defense;
        renderDebugTextWindowRightAligned(
            textureManager,
            defText.str(),
            layout.windowSkillsRightPane,
            layout.skillsStatValueX,
            layout.skillsStatAlignWidth,
            layout.skillsDefenseY,
            scale,
            offsetX,
            offsetY
        );
        std::ostringstream spaText;
        spaText << stats.spAttack;
        renderDebugTextWindowRightAligned(
            textureManager,
            spaText.str(),
            layout.windowSkillsRightPane,
            layout.skillsStatValueX,
            layout.skillsStatAlignWidth,
            layout.skillsSpAtkY,
            scale,
            offsetX,
            offsetY
        );
        std::ostringstream spdText;
        spdText << stats.spDefense;
        renderDebugTextWindowRightAligned(
            textureManager,
            spdText.str(),
            layout.windowSkillsRightPane,
            layout.skillsStatValueX,
            layout.skillsStatAlignWidth,
            layout.skillsSpDefY,
            scale,
            offsetX,
            offsetY
        );
        std::ostringstream speText;
        speText << stats.speed;
        renderDebugTextWindowRightAligned(
            textureManager,
            speText.str(),
            layout.windowSkillsRightPane,
            layout.skillsStatValueX,
            layout.skillsStatAlignWidth,
            layout.skillsSpeedY,
            scale,
            offsetX,
            offsetY
        );

        std::ostringstream expText;
        expText << stats.expPoints;
        renderDebugTextWindowRightAligned(
            textureManager,
            expText.str(),
            layout.windowSkillsRightPane,
            layout.skillsExpValueX,
            layout.skillsExpAlignWidth,
            layout.skillsExpPointsY,
            scale,
            offsetX,
            offsetY
        );
        std::ostringstream nextText;
        nextText << stats.expToNextLevel;
        renderDebugTextWindowRightAligned(
            textureManager,
            nextText.str(),
            layout.windowSkillsRightPane,
            layout.skillsExpValueX,
            layout.skillsExpAlignWidth,
            layout.skillsNextLevelY,
            scale,
            offsetX,
            offsetY
        );

        renderDebugTextWindowAligned(
            textureManager,
            "EXP. POINTS",
            layout.windowSkillsExpText,
            layout.skillsExpLabelX,
            layout.skillsExpLabelY,
            scale,
            offsetX,
            offsetY
        );
        renderDebugTextWindowAligned(
            textureManager,
            "NEXT LV.",
            layout.windowSkillsExpText,
            layout.skillsNextLabelX,
            layout.skillsNextLabelY,
            scale,
            offsetX,
            offsetY
        );

        renderDebugTextWindowAligned(
            textureManager,
            abilityNameFor(member),
            layout.windowSkillsAbility,
            layout.skillsAbilityNameX,
            layout.skillsAbilityNameY,
            scale,
            offsetX,
            offsetY
        );
        renderDebugTextWindowAligned(
            textureManager,
            abilityDescriptionFor(member),
            layout.windowSkillsAbility,
            layout.skillsAbilityDescX,
            layout.skillsAbilityDescY,
            scale,
            offsetX,
            offsetY
        );

        renderSegmentedGauge(
            renderer,
            textureManager.load(kAssetExpBar),
            layout.skillsExpGaugeOrigin,
            layout.skillsExpGaugeSegments,
            stats.expRatio,
            scale,
            offsetX,
            offsetY
        );
    } else {
        const auto moves = movesFor(member);
        for (int i = 0; i < 4; ++i) {
            const MoveDisplay& move = moves[static_cast<std::size_t>(i)];
            const float nameY = (layout.movesRowStepY * static_cast<float>(i)) + layout.movesNameBaseY;
            const float ppY = nameY + layout.movesPpYOffset;
            const int ppColorIndex = movePpColorIndex(move.pp, move.maxPp);
            const SDL_Color ppTextColor = movePpColorFromIndex(ppColorIndex);
            const SDL_Color ppShadowColor = movePpShadowColorFromIndex(ppColorIndex);

            renderDebugTextWindowAligned(
                textureManager,
                move.name,
                layout.windowMovesRightPane,
                layout.movesNameX,
                nameY,
                scale,
                offsetX,
                offsetY,
                FrlgTextRenderer::Font::Normal,
                kMoveTextColorNormal,
                kMoveTextColorNormalShadow
            );
            std::ostringstream ppText;
            ppText << "PP" << move.pp << "/" << move.maxPp;
            renderDebugTextWindowRightAligned(
                textureManager,
                ppText.str(),
                layout.windowMovesRightPane,
                0.0f,
                79.0f,
                ppY,
                scale,
                offsetX,
                offsetY,
                FrlgTextRenderer::Font::Normal,
                ppTextColor,
                ppShadowColor
            );

            renderTypeIcon(
                renderer,
                menuInfoTexture,
                move.typeId,
                SDL_FRect{
                    layout.windowMovesTypePane.x + layout.movesTypeIconXOffset,
                    layout.windowMovesTypePane.y + nameY,
                    layout.movesTypeIconWidth,
                    layout.movesTypeIconHeight
                },
                scale,
                offsetX,
                offsetY
            );
        }
    }
}

int PokemonSummaryOverlay::partyCount(const std::vector<PartyPokemon>& party) const {
    return std::min<int>(kMaxPartySlots, static_cast<int>(party.size()));
}

void PokemonSummaryOverlay::moveMonSelection(const SDL_Keycode key, const int count) {
    if (count <= 1) {
        selectedIndex_ = 0;
        return;
    }

    if (key == SDLK_UP || key == SDLK_W) {
        selectedIndex_ = (selectedIndex_ == 0) ? (count - 1) : (selectedIndex_ - 1);
    } else if (key == SDLK_DOWN || key == SDLK_S) {
        selectedIndex_ = (selectedIndex_ + 1) % count;
    }
}

void PokemonSummaryOverlay::flipPage(const SDL_Keycode key) {
    if (key == SDLK_LEFT || key == SDLK_A) {
        if (page_ == SummaryPage::Info) {
            page_ = SummaryPage::Moves;
        } else if (page_ == SummaryPage::Skills) {
            page_ = SummaryPage::Info;
        } else {
            page_ = SummaryPage::Skills;
        }
    } else if (key == SDLK_RIGHT || key == SDLK_D) {
        if (page_ == SummaryPage::Info) {
            page_ = SummaryPage::Skills;
        } else if (page_ == SummaryPage::Skills) {
            page_ = SummaryPage::Moves;
        } else {
            page_ = SummaryPage::Info;
        }
    }
}

PokemonSummaryOverlay::DisplayStats PokemonSummaryOverlay::makeDisplayStats(const PartyPokemon& member) {
    DisplayStats stats{};
    stats.maxHp = std::max(1, 12 + (member.level * 3));
    stats.hp = stats.maxHp;
    stats.attack = std::max(1, 5 + (member.level * 2));
    stats.defense = std::max(1, 5 + (member.level * 2));
    stats.speed = std::max(1, 5 + (member.level * 2));
    stats.spAttack = std::max(1, 5 + (member.level * 2));
    stats.spDefense = std::max(1, 5 + (member.level * 2));
    stats.expPoints = member.level * member.level * member.level;

    const int nextLevel = std::min(100, member.level + 1);
    const int nextLevelExp = nextLevel * nextLevel * nextLevel;
    stats.expToNextLevel = std::max(0, nextLevelExp - stats.expPoints);

    const int currentLevelBase = member.level * member.level * member.level;
    const int currentLevelCap = nextLevelExp;
    const int levelSpan = std::max(1, currentLevelCap - currentLevelBase);
    stats.expRatio = std::clamp(
        static_cast<float>(stats.expPoints - currentLevelBase) / static_cast<float>(levelSpan),
        0.0f,
        1.0f
    );
    stats.hpRatio = std::clamp(static_cast<float>(stats.hp) / static_cast<float>(stats.maxHp), 0.0f, 1.0f);
    return stats;
}
