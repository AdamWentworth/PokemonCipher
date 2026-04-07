#include "TextureManager.h"
#include "Game.h"

#include <filesystem>
#include <iostream>

extern Game* game;
std::unordered_map<std::string, SDL_Texture*> TextureManager::textures;

static std::filesystem::path GetExeDir()
{
    const char* base = SDL_GetBasePath();
    if (!base || base[0] == '\0') {
        return std::filesystem::current_path();
    }
    return std::filesystem::path(base);
}

SDL_Texture* TextureManager::load(const char* path)
{   
    //Caching: storing the result of some work so you don't have to repeat the work next time
    //Check if the texture already exists in the map
    auto it = textures.find(path);
    if (it != textures.end()) {
        return it->second;
    }

    std::filesystem::path input(path);  
    std::filesystem::path fullPath =
        input.is_absolute() ? input : (GetExeDir() / input);

    SDL_Surface* tempSurface = IMG_Load(fullPath.string().c_str());
    if (!tempSurface) {
        std::cout << "Failed to load image: " << fullPath.string() << "\n";
        std::cout << "SDL error: " << SDL_GetError() << "\n";
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(game->renderer, tempSurface);
    SDL_DestroySurface(tempSurface);

    //check if the texture creation was successful
    if (!texture) {
        std::cout << "Failed to create texture: " << path << std::endl;
        return nullptr;
    }

    // fixes tile seams by forcing pixel art to scale with nearest
    // sampling, which stops SDL from blending colors from neighboring tiles.
    if (!SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST)) {
        std::cout << "Failed to set nearest scale mode for: " << fullPath.string() << "\n";
        std::cout << "SDL error: " << SDL_GetError() << "\n";
    }

    // store new texture in the cache
    textures[path] = texture;

    if (!texture) {
        std::cout << "Failed to create texture from: " << fullPath.string() << "\n";
        std::cout << "SDL error: " << SDL_GetError() << "\n";
        return nullptr;
    }

    return texture;
}

void TextureManager::draw(SDL_Texture* texture, SDL_FRect src, SDL_FRect dst)
{
    if (!texture) return;
    SDL_RenderTexture(game->renderer, texture, &src, &dst);
};

void TextureManager::clean() {
    for (auto& tex: textures) {
        SDL_DestroyTexture(tex.second);
        tex.second = nullptr;
    }
    textures.clear();
};