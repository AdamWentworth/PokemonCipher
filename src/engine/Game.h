#pragma once

#include <SDL3/SDL.h>

#include <memory>

#include "TextureManager.h"
#include "manager/SceneManager.h"

class Game {
public:
    Game() = default;
    ~Game();

    bool init(const char* title, int width, int height, bool fullscreen);
    bool setLogicalPresentation(int width, int height, SDL_RendererLogicalPresentation mode);
    void handleEvents();
    void update(float deltaTime);
    void render();
    void destroy();

    bool running() const { return isRunning_; }

    SceneManager& sceneManager() { return sceneManager_; }
    TextureManager& textureManager() { return *textureManager_; }

private:
    bool isRunning_ = false;

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    SceneManager sceneManager_;
    std::unique_ptr<TextureManager> textureManager_;
};
