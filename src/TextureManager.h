#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>
#include <unordered_map>

class TextureManager {
    static std::unordered_map<std::string, SDL_Texture*> textures;

public:

    static SDL_Texture* load(const char* path);
    static void draw(SDL_Texture* texture, SDL_FRect src, SDL_FRect dst);
    static void clean();
};
