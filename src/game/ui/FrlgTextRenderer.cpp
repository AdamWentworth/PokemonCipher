#include "game/ui/FrlgTextRenderer.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <string>

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3_image/SDL_image.h>

#include "engine/TextureManager.h"

namespace {
constexpr std::uint8_t kGlyphSpace = 0x00;
constexpr std::uint8_t kGlyphQuestion = 0xAC;
constexpr const char* kKeypadIconsTexturePath = "assets/fonts/keypad_icons.png";
constexpr std::uint8_t kDarkGlyphChannelValue = 56;
constexpr std::uint8_t kLightGlyphChannelValue = 216;

std::uint32_t packColor(const SDL_Color color) {
    return (static_cast<std::uint32_t>(color.r) << 24) |
        (static_cast<std::uint32_t>(color.g) << 16) |
        (static_cast<std::uint32_t>(color.b) << 8) |
        static_cast<std::uint32_t>(color.a);
}

std::uint64_t makeTintedTextureKey(const SDL_Color foregroundColor, const SDL_Color shadowColor) {
    return (static_cast<std::uint64_t>(packColor(foregroundColor)) << 32) |
        static_cast<std::uint64_t>(packColor(shadowColor));
}

SDL_FRect keypadIconSourceRect(const char marker) {
    struct IconSpec {
        int tileOffset = 0;
        int width = 8;
        int height = 12;
    };

    constexpr int kTileSize = 8;
    constexpr int kTilesPerRow = 16;

    IconSpec spec{};
    bool hasSpec = true;
    switch (marker) {
    case '@': // {A_BUTTON}
        spec = IconSpec{0x00, 8, 12};
        break;
    case '#': // {B_BUTTON}
        spec = IconSpec{0x01, 8, 12};
        break;
    case '<': // {DPAD_LEFT}
        spec = IconSpec{0x0E, 8, 12};
        break;
    case '>': // {DPAD_RIGHT}
        spec = IconSpec{0x0F, 8, 12};
        break;
    case '=': // {DPAD_LEFTRIGHT}
        spec = IconSpec{0x21, 8, 12};
        break;
    case '~': // {DPAD_UPDOWN}
        spec = IconSpec{0x20, 8, 12};
        break;
    default:
        hasSpec = false;
        break;
    }

    if (!hasSpec) {
        return SDL_FRect{};
    }

    // PNG export of keypad icons is shifted down by 4 px relative to
    // the original 8x8 tile grid indexing used in FRLG text.c offsets.
    return SDL_FRect{
        static_cast<float>((spec.tileOffset % kTilesPerRow) * kTileSize),
        static_cast<float>(((spec.tileOffset / kTilesPerRow) * kTileSize) + 4),
        static_cast<float>(spec.width),
        static_cast<float>(spec.height),
    };
}

bool isKeypadMarker(const char ch) {
    return ch == '@' || ch == '#' || ch == '<' || ch == '>' || ch == '=' || ch == '~';
}
} // namespace

float FrlgTextRenderer::measureText(TextureManager& textureManager, std::string_view text, const Font font) const {
    FontState& state = fontState(font);
    initializeMetrics(textureManager, state);

    float currentLineWidth = 0.0f;
    float maxLineWidth = 0.0f;
    for (const char ch : text) {
        if (ch == '\n') {
            maxLineWidth = std::max(maxLineWidth, currentLineWidth);
            currentLineWidth = 0.0f;
            continue;
        }

        if (isKeypadMarker(ch)) {
            const SDL_FRect src = keypadIconSourceRect(ch);
            if (src.w > 0.0f) {
                currentLineWidth += (src.w + 1.0f);
                continue;
            }
        }

        const std::uint8_t glyph = mapAsciiToGlyph(ch);
        const GlyphMetrics& metrics = state.metrics[glyph];
        currentLineWidth += static_cast<float>(metrics.advance) * state.sourceScale;
    }

    return std::max(maxLineWidth, currentLineWidth);
}

void FrlgTextRenderer::drawText(
    TextureManager& textureManager,
    std::string_view text,
    const float screenX,
    const float screenY,
    const float scale,
    const Font font,
    const SDL_Color foregroundColor,
    const SDL_Color shadowColor
) const {
    SDL_Renderer* renderer = textureManager.renderer();
    if (!renderer || scale <= 0.0f) {
        return;
    }

    FontState& state = fontState(font);
    initializeMetrics(textureManager, state);

    SDL_Texture* texture = getTintedTexture(textureManager, state, foregroundColor, shadowColor);
    if (!texture) {
        return;
    }

    const float displayScale = state.sourceScale * scale;
    float penX = screenX;
    float penY = screenY;
    const float lineHeight = static_cast<float>(state.cellHeight) * displayScale;
    SDL_Texture* keypadTexture = nullptr;

    for (const char ch : text) {
        if (ch == '\n') {
            penX = screenX;
            penY += lineHeight;
            continue;
        }

        if (isKeypadMarker(ch)) {
            if (!keypadTexture) {
                keypadTexture = textureManager.load(kKeypadIconsTexturePath);
            }

            const SDL_FRect src = keypadIconSourceRect(ch);
            if (keypadTexture && src.w > 0.0f && src.h > 0.0f) {
                const SDL_FRect dst{
                    penX,
                    penY,
                    src.w * scale,
                    src.h * scale
                };
                SDL_RenderTexture(renderer, keypadTexture, &src, &dst);
                penX += (src.w + 1.0f) * scale;
                continue;
            }
        }

        const std::uint8_t glyph = mapAsciiToGlyph(ch);
        const GlyphMetrics& metrics = state.metrics[glyph];
        const float advance = static_cast<float>(metrics.advance) * displayScale;

        if (metrics.hasPixels) {
            const int glyphIndex = static_cast<int>(glyph);
            const int cellX = (glyphIndex % state.columns) * state.cellWidth;
            const int cellY = (glyphIndex / state.columns) * state.cellHeight;
            const int srcWidth = static_cast<int>(metrics.maxX - metrics.minX + 1);

            const SDL_FRect srcRect{
                static_cast<float>(cellX + metrics.minX),
                static_cast<float>(cellY),
                static_cast<float>(srcWidth),
                static_cast<float>(state.cellHeight)
            };
            const SDL_FRect dstRect{
                penX,
                penY,
                static_cast<float>(srcWidth) * displayScale,
                static_cast<float>(state.cellHeight) * displayScale
            };
            SDL_RenderTexture(renderer, texture, &srcRect, &dstRect);
        }

        penX += advance;
    }
}

std::uint8_t FrlgTextRenderer::mapAsciiToGlyph(const char ch) {
    if (ch == ' ') {
        return kGlyphSpace;
    }

    if (ch >= '0' && ch <= '9') {
        return static_cast<std::uint8_t>(0xA1 + (ch - '0'));
    }

    if (ch >= 'A' && ch <= 'Z') {
        return static_cast<std::uint8_t>(0xBB + (ch - 'A'));
    }

    if (ch >= 'a' && ch <= 'z') {
        return static_cast<std::uint8_t>(0xD5 + (ch - 'a'));
    }

    switch (ch) {
    case '!':
        return 0xAB;
    case '?':
        return 0xAC;
    case '.':
        return 0xAD;
    case '-':
        return 0xAE;
    case ',':
        return 0xB8;
    case '/':
        return 0xBA;
    case ':':
        return 0xF0;
    case ';':
        return 0x36;
    case '(':
        return 0x5C;
    case ')':
        return 0x5D;
    case '\'':
        return 0xB4;
    case '+':
        return 0x2E;
    case '=':
        return 0x35;
    case '^': // male symbol marker for FRLG text
        return 0xB5;
    case '&': // female symbol marker for FRLG text
        return 0xB6;
    default:
        return kGlyphQuestion;
    }
}

FrlgTextRenderer::FontState& FrlgTextRenderer::fontState(const Font font) {
    static FontState normal{
        .texturePath = "assets/fonts/latin_normal.png",
        .columns = 16,
        .cellWidth = 16,
        .cellHeight = 16,
        .spaceAdvance = 8,
        .sourceScale = 0.5f,
    };

    static FontState small{
        .texturePath = "assets/fonts/latin_small.png",
        .columns = 16,
        .cellWidth = 16,
        .cellHeight = 16,
        .spaceAdvance = 7,
        .sourceScale = 0.5f,
    };

    return font == Font::Small ? small : normal;
}

std::filesystem::path FrlgTextRenderer::resolvePath(const std::string& path) {
    const std::filesystem::path input(path);
    if (input.is_absolute()) {
        return input;
    }

    const auto findInAncestors = [&input](const std::filesystem::path& start, const bool includeStart)
        -> std::filesystem::path {
        if (start.empty()) {
            return {};
        }

        std::filesystem::path cursor = includeStart ? start : start.parent_path();
        while (!cursor.empty()) {
            const std::filesystem::path candidate = cursor / input;
            if (std::filesystem::exists(candidate)) {
                return candidate;
            }

            const std::filesystem::path parent = cursor.parent_path();
            if (parent == cursor) {
                break;
            }
            cursor = parent;
        }

        return {};
    };

    const std::filesystem::path currentDirectory = std::filesystem::current_path();
    if (const std::filesystem::path fromCurrentAncestors = findInAncestors(currentDirectory, false);
        !fromCurrentAncestors.empty()) {
        return fromCurrentAncestors;
    }
    if (const std::filesystem::path fromCurrent = findInAncestors(currentDirectory, true);
        !fromCurrent.empty()) {
        return fromCurrent;
    }

    const char* basePath = SDL_GetBasePath();
    std::filesystem::path executableDirectory;
    if (basePath && basePath[0] != '\0') {
        executableDirectory = std::filesystem::path(basePath);
    } else {
        executableDirectory = currentDirectory;
    }

    if (const std::filesystem::path fromExecutableAncestors = findInAncestors(executableDirectory, false);
        !fromExecutableAncestors.empty()) {
        return fromExecutableAncestors;
    }
    if (const std::filesystem::path fromExecutable = findInAncestors(executableDirectory, true);
        !fromExecutable.empty()) {
        return fromExecutable;
    }

    return executableDirectory / input;
}

void FrlgTextRenderer::initializeMetrics(TextureManager& textureManager, FontState& state) {
    if (state.initialized) {
        return;
    }

    textureManager.load(state.texturePath);

    const std::filesystem::path fullPath = resolvePath(state.texturePath);
    SDL_Surface* surface = IMG_Load(fullPath.string().c_str());
    if (!surface) {
        std::cout << "Failed to load font surface '" << fullPath.string() << "': " << SDL_GetError() << '\n';
        state.initialized = true;
        return;
    }

    std::uint8_t bgR = 0;
    std::uint8_t bgG = 0;
    std::uint8_t bgB = 0;
    std::uint8_t bgA = 0;
    if (!SDL_ReadSurfacePixel(surface, 0, 0, &bgR, &bgG, &bgB, &bgA)) {
        SDL_DestroySurface(surface);
        state.initialized = true;
        return;
    }

    const int width = surface->w;
    const int height = surface->h;

    for (int glyphIndex = 0; glyphIndex < static_cast<int>(state.metrics.size()); ++glyphIndex) {
        const int cellX = (glyphIndex % state.columns) * state.cellWidth;
        const int cellY = (glyphIndex / state.columns) * state.cellHeight;

        GlyphMetrics metrics{};
        metrics.advance = static_cast<std::uint8_t>(state.spaceAdvance);

        if (cellX + state.cellWidth > width || cellY + state.cellHeight > height) {
            state.metrics[static_cast<std::size_t>(glyphIndex)] = metrics;
            continue;
        }

        int minX = state.cellWidth;
        int maxX = -1;
        for (int y = 0; y < state.cellHeight; ++y) {
            for (int x = 0; x < state.cellWidth; ++x) {
                std::uint8_t r = 0;
                std::uint8_t g = 0;
                std::uint8_t b = 0;
                std::uint8_t a = 0;
                if (!SDL_ReadSurfacePixel(surface, cellX + x, cellY + y, &r, &g, &b, &a)) {
                    continue;
                }

                if (a == 0) {
                    continue;
                }

                if (r == bgR && g == bgG && b == bgB) {
                    continue;
                }

                minX = std::min(minX, x);
                maxX = std::max(maxX, x);
            }
        }

        if (maxX >= minX) {
            const int span = (maxX - minX) + 1;
            metrics.hasPixels = true;
            metrics.minX = static_cast<std::uint8_t>(minX);
            metrics.maxX = static_cast<std::uint8_t>(maxX);
            metrics.advance = static_cast<std::uint8_t>(std::min(state.cellWidth, span + 1));
        }

        if (glyphIndex == static_cast<int>(kGlyphSpace)) {
            metrics.hasPixels = false;
            metrics.minX = 0;
            metrics.maxX = 0;
            metrics.advance = static_cast<std::uint8_t>(state.spaceAdvance);
        }

        state.metrics[static_cast<std::size_t>(glyphIndex)] = metrics;
    }

    SDL_DestroySurface(surface);
    state.initialized = true;
}

SDL_Texture* FrlgTextRenderer::getTintedTexture(
    TextureManager& textureManager,
    FontState& state,
    const SDL_Color foregroundColor,
    const SDL_Color shadowColor
) {
    const std::uint64_t cacheKey = makeTintedTextureKey(foregroundColor, shadowColor);
    if (const auto cached = state.tintedTextureCache.find(cacheKey); cached != state.tintedTextureCache.end()) {
        return cached->second;
    }

    SDL_Renderer* renderer = textureManager.renderer();
    if (!renderer) {
        return nullptr;
    }

    const std::filesystem::path fullPath = resolvePath(state.texturePath);
    SDL_Surface* sourceSurface = IMG_Load(fullPath.string().c_str());
    if (!sourceSurface) {
        std::cout << "Failed to load font surface '" << fullPath.string() << "': " << SDL_GetError() << '\n';
        return nullptr;
    }

    SDL_Surface* rgbaSurface = SDL_ConvertSurface(sourceSurface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(sourceSurface);
    if (!rgbaSurface) {
        std::cout << "Failed to convert font surface '" << fullPath.string() << "': " << SDL_GetError() << '\n';
        return nullptr;
    }

    for (int y = 0; y < rgbaSurface->h; ++y) {
        for (int x = 0; x < rgbaSurface->w; ++x) {
            std::uint8_t r = 0;
            std::uint8_t g = 0;
            std::uint8_t b = 0;
            std::uint8_t a = 0;
            if (!SDL_ReadSurfacePixel(rgbaSurface, x, y, &r, &g, &b, &a)) {
                continue;
            }

            if (a == 0) {
                continue;
            }

            const int distToDark = std::abs(static_cast<int>(r) - static_cast<int>(kDarkGlyphChannelValue)) +
                std::abs(static_cast<int>(g) - static_cast<int>(kDarkGlyphChannelValue)) +
                std::abs(static_cast<int>(b) - static_cast<int>(kDarkGlyphChannelValue));
            const int distToLight = std::abs(static_cast<int>(r) - static_cast<int>(kLightGlyphChannelValue)) +
                std::abs(static_cast<int>(g) - static_cast<int>(kLightGlyphChannelValue)) +
                std::abs(static_cast<int>(b) - static_cast<int>(kLightGlyphChannelValue));

            // Font atlas uses two shades:
            // - darker shade (56) = main glyph body (foreground)
            // - lighter shade (216) = edge highlight/shadow channel
            const SDL_Color target = (distToLight <= distToDark) ? shadowColor : foregroundColor;
            SDL_WriteSurfacePixel(rgbaSurface, x, y, target.r, target.g, target.b, a);
        }
    }

    SDL_Texture* tintedTexture = SDL_CreateTextureFromSurface(renderer, rgbaSurface);
    SDL_DestroySurface(rgbaSurface);
    if (!tintedTexture) {
        std::cout << "Failed to create tinted font texture '" << fullPath.string() << "': " << SDL_GetError() << '\n';
        return nullptr;
    }

    SDL_SetTextureScaleMode(tintedTexture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(tintedTexture, SDL_BLENDMODE_BLEND);
    state.tintedTextureCache.emplace(cacheKey, tintedTexture);
    return tintedTexture;
}
