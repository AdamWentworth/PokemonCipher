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
    // Prefer source-tree assets when running from a build directory.
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

SDL_Renderer* TextureManager::renderer() const {
    return renderer_;
}

void TextureManager::clear() {
    for (auto& [_, texture] : textures_) {
        if (texture) {
            SDL_DestroyTexture(texture);
        }
    }

    textures_.clear();
}
