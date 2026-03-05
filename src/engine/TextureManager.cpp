#include "TextureManager.h"

#include <iostream>

TextureManager::TextureManager(SDL_Renderer* renderer)
    : renderer_(renderer) {}

TextureManager::~TextureManager() {
    clear();
}

std::filesystem::path TextureManager::resolvePath(const std::string& path) const {
    const std::filesystem::path input(path);
    if (input.is_absolute()) {
        return input;
    }

    const char* basePath = SDL_GetBasePath();
    std::filesystem::path executableDirectory;
    if (basePath && basePath[0] != '\0') {
        executableDirectory = std::filesystem::path(basePath);
    } else {
        executableDirectory = std::filesystem::current_path();
    }

    return executableDirectory / input;
}

SDL_Texture* TextureManager::load(const std::string& path) {
    const auto cached = textures_.find(path);
    if (cached != textures_.end()) {
        return cached->second;
    }

    const std::filesystem::path fullPath = resolvePath(path);
    SDL_Surface* surface = IMG_Load(fullPath.string().c_str());
    if (!surface) {
        std::cout << "Failed to load image '" << fullPath.string() << "': " << SDL_GetError() << '\n';
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_DestroySurface(surface);

    if (!texture) {
        std::cout << "Failed to create texture for '" << fullPath.string() << "': " << SDL_GetError() << '\n';
        return nullptr;
    }

    if (!SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST)) {
        std::cout << "Failed to set nearest scale mode for '" << fullPath.string() << "': " << SDL_GetError() << '\n';
    }

    textures_[path] = texture;
    return texture;
}

void TextureManager::draw(
    SDL_Texture* texture,
    const SDL_FRect& src,
    const SDL_FRect& dst,
    SDL_FlipMode flip
) const {
    if (!texture) {
        return;
    }

    if (flip == SDL_FLIP_NONE) {
        SDL_RenderTexture(renderer_, texture, &src, &dst);
        return;
    }

    SDL_RenderTextureRotated(renderer_, texture, &src, &dst, 0.0, nullptr, flip);
}

void TextureManager::clear() {
    for (auto& [_, texture] : textures_) {
        if (texture) {
            SDL_DestroyTexture(texture);
        }
    }

    textures_.clear();
}
