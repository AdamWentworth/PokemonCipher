#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <unordered_map>
#include <string_view>

#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>

class TextureManager;

class FrlgTextRenderer {
public:
    enum class Font {
        Normal,
        Small
    };

    float measureText(TextureManager& textureManager, std::string_view text, Font font) const;
    void drawText(
        TextureManager& textureManager,
        std::string_view text,
        float screenX,
        float screenY,
        float scale,
        Font font,
        SDL_Color foregroundColor = SDL_Color{255, 255, 255, 255},
        SDL_Color shadowColor = SDL_Color{98, 98, 98, 255}
    ) const;

private:
    struct GlyphMetrics {
        std::uint8_t minX = 0;
        std::uint8_t maxX = 0;
        std::uint8_t advance = 8;
        bool hasPixels = false;
    };

    struct FontState {
        const char* texturePath = nullptr;
        int columns = 16;
        int cellWidth = 16;
        int cellHeight = 16;
        int spaceAdvance = 8;
        float sourceScale = 0.5f;
        mutable bool initialized = false;
        mutable std::array<GlyphMetrics, 256> metrics{};
        mutable std::unordered_map<std::uint64_t, SDL_Texture*> tintedTextureCache{};
    };

    static std::uint8_t mapAsciiToGlyph(char ch);
    static FontState& fontState(Font font);
    static std::filesystem::path resolvePath(const std::string& path);
    static void initializeMetrics(TextureManager& textureManager, FontState& state);
    static SDL_Texture* getTintedTexture(
        TextureManager& textureManager,
        FontState& state,
        SDL_Color foregroundColor,
        SDL_Color shadowColor
    );
};
