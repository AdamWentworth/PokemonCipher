#pragma once

#include <string_view>

#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>

#include "game/ui/FrlgTextRenderer.h"

class TextureManager;
struct PokemonSummaryLayout;
struct SDL_Renderer;
struct SDL_Texture;

namespace summary_render {

inline constexpr SDL_Color kTextColorDark{65, 65, 65, 255};
inline constexpr SDL_Color kTextColorDarkShadow{222, 222, 197, 255};
inline constexpr SDL_Color kMoveTextColorNormal{32, 32, 32, 255};
inline constexpr SDL_Color kMoveTextColorNormalShadow{222, 222, 222, 255};
inline constexpr SDL_Color kMoveTextColorLow{238, 222, 0, 255};
inline constexpr SDL_Color kMoveTextColorLowShadow{255, 246, 139, 255};
inline constexpr SDL_Color kMoveTextColorVeryLow{255, 148, 0, 255};
inline constexpr SDL_Color kMoveTextColorVeryLowShadow{255, 238, 115, 255};
inline constexpr SDL_Color kMoveTextColorEmpty{238, 0, 0, 255};
inline constexpr SDL_Color kMoveTextColorEmptyShadow{246, 222, 156, 255};

SDL_FRect mapToViewport(const SDL_FRect& rect, float scale, float offsetX, float offsetY);

float rightAlignOffset(
    TextureManager& textureManager,
    const PokemonSummaryLayout& layout,
    std::string_view text,
    float alignWidth,
    FrlgTextRenderer::Font font
);

void renderDebugText(
    TextureManager& textureManager,
    std::string_view text,
    float x,
    float y,
    float scale,
    float offsetX,
    float offsetY,
    const PokemonSummaryLayout& layout,
    FrlgTextRenderer::Font font = FrlgTextRenderer::Font::Normal,
    SDL_Color color = kTextColorDark,
    SDL_Color shadowColor = kTextColorDarkShadow
);

void renderDebugTextWindowAligned(
    TextureManager& textureManager,
    std::string_view text,
    const SDL_FPoint& windowOrigin,
    float x,
    float y,
    float scale,
    float offsetX,
    float offsetY,
    const PokemonSummaryLayout& layout,
    FrlgTextRenderer::Font font = FrlgTextRenderer::Font::Normal,
    SDL_Color color = kTextColorDark,
    SDL_Color shadowColor = kTextColorDarkShadow
);

void renderDebugTextWindowRightAligned(
    TextureManager& textureManager,
    std::string_view text,
    const SDL_FPoint& windowOrigin,
    float x,
    float alignWidth,
    float y,
    float scale,
    float offsetX,
    float offsetY,
    const PokemonSummaryLayout& layout,
    FrlgTextRenderer::Font font = FrlgTextRenderer::Font::Normal,
    SDL_Color color = kTextColorDark,
    SDL_Color shadowColor = kTextColorDarkShadow
);

void renderTypeIcon(
    SDL_Renderer* renderer,
    SDL_Texture* menuInfoTexture,
    int typeId,
    const SDL_FRect& designDst,
    float scale,
    float offsetX,
    float offsetY
);

void renderSegmentedGauge(
    SDL_Renderer* renderer,
    SDL_Texture* gaugeTexture,
    SDL_FPoint origin,
    int middleSegments,
    float ratio,
    float scale,
    float offsetX,
    float offsetY,
    const PokemonSummaryLayout& layout
);

SDL_Color movePpColorFromIndex(int colorIndex);
SDL_Color movePpShadowColorFromIndex(int colorIndex);

} // namespace summary_render

