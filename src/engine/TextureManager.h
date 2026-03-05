#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <filesystem>
#include <string>
#include <unordered_map>

class TextureManager {
public:
    explicit TextureManager(SDL_Renderer* renderer);
    ~TextureManager();

    SDL_Texture* load(const std::string& path);
    void draw(
        SDL_Texture* texture,
        const SDL_FRect& src,
        const SDL_FRect& dst,
        SDL_FlipMode flip = SDL_FLIP_NONE
    ) const;
    SDL_Renderer* renderer() const;
    void clear();

private:
    std::filesystem::path resolvePath(const std::string& path) const;

    SDL_Renderer* renderer_ = nullptr;
    std::unordered_map<std::string, SDL_Texture*> textures_;
};
