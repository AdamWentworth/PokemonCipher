#include "game/ui/PokemonSummaryRenderUtils.h"

#include <algorithm>
#include <cmath>

#include <SDL3/SDL_render.h>

#include "engine/TextureManager.h"
#include "game/ui/PokemonSummaryLayout.h"

namespace {

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

FrlgTextRenderer& summaryTextRenderer() {
    static FrlgTextRenderer renderer;
    return renderer;
}

float mapX(const float x, const float scale, const float offsetX) {
    return offsetX + (x * scale);
}

float mapY(
    const float y,
    const float scale,
    const float offsetY,
    const PokemonSummaryLayout& layout
) {
    return offsetY + ((y + layout.textBaselineYOffset) * scale);
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

} // namespace

namespace summary_render {

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

float rightAlignOffset(
    TextureManager& textureManager,
    const PokemonSummaryLayout& layout,
    const std::string_view text,
    const float alignWidth,
    const FrlgTextRenderer::Font font
) {
    const float textWidth =
        summaryTextRenderer().measureText(textureManager, text, font) * layout.textScaleMultiplier;
    return std::max(0.0f, alignWidth - textWidth);
}

void renderDebugText(
    TextureManager& textureManager,
    const std::string_view text,
    const float x,
    const float y,
    const float scale,
    const float offsetX,
    const float offsetY,
    const PokemonSummaryLayout& layout,
    const FrlgTextRenderer::Font font,
    const SDL_Color color,
    const SDL_Color shadowColor
) {
    const float textScale = scale * layout.textScaleMultiplier;
    summaryTextRenderer().drawText(
        textureManager,
        text,
        mapX(x, scale, offsetX),
        mapY(y, scale, offsetY, layout),
        textScale,
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
    const PokemonSummaryLayout& layout,
    const FrlgTextRenderer::Font font,
    const SDL_Color color,
    const SDL_Color shadowColor
) {
    renderDebugText(
        textureManager,
        text,
        windowOrigin.x + x,
        windowOrigin.y + y,
        scale,
        offsetX,
        offsetY,
        layout,
        font,
        color,
        shadowColor
    );
}

void renderDebugTextWindowRightAligned(
    TextureManager& textureManager,
    const std::string_view text,
    const SDL_FPoint& windowOrigin,
    const float x,
    const float alignWidth,
    const float y,
    const float scale,
    const float offsetX,
    const float offsetY,
    const PokemonSummaryLayout& layout,
    const FrlgTextRenderer::Font font,
    const SDL_Color color,
    const SDL_Color shadowColor
) {
    renderDebugTextWindowAligned(
        textureManager,
        text,
        windowOrigin,
        x + rightAlignOffset(textureManager, layout, text, alignWidth, font),
        y,
        scale,
        offsetX,
        offsetY,
        layout,
        font,
        color,
        shadowColor
    );
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

void renderSegmentedGauge(
    SDL_Renderer* renderer,
    SDL_Texture* gaugeTexture,
    const SDL_FPoint origin,
    const int middleSegments,
    float ratio,
    const float scale,
    const float offsetX,
    const float offsetY,
    const PokemonSummaryLayout& layout
) {
    if (!renderer || middleSegments <= 0) {
        return;
    }

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

} // namespace summary_render

