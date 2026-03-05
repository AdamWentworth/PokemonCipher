#include "Game.h"

#include <iostream>

Game::~Game() {
    destroy();
}

bool Game::init(const char* title, const int width, const int height, const bool fullscreen) {
    const SDL_WindowFlags flags = fullscreen ? SDL_WINDOW_FULLSCREEN : 0;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "SDL video init failed: " << SDL_GetError() << '\n';
        return false;
    }

    window_ = SDL_CreateWindow(title, width, height, flags);
    if (!window_) {
        std::cout << "Window could not be created: " << SDL_GetError() << '\n';
        destroy();
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (!renderer_) {
        std::cout << "Renderer could not be created: " << SDL_GetError() << '\n';
        destroy();
        return false;
    }

    textureManager_ = std::make_unique<TextureManager>(renderer_);
    isRunning_ = true;
    return true;
}

bool Game::setLogicalPresentation(const int width, const int height, const SDL_RendererLogicalPresentation mode) {
    if (!renderer_) {
        return false;
    }

    if (!SDL_SetRenderLogicalPresentation(renderer_, width, height, mode)) {
        std::cout << "Failed to set logical presentation: " << SDL_GetError() << '\n';
        return false;
    }

    return true;
}

void Game::handleEvents() {
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            isRunning_ = false;
            return;
        }

        sceneManager_.handleEvent(event);
    }
}

void Game::update(const float deltaTime) {
    sceneManager_.update(deltaTime);
}

void Game::render() {
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);

    sceneManager_.render();

    SDL_RenderPresent(renderer_);
}

void Game::destroy() {
    isRunning_ = false;

    textureManager_.reset();

    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }

    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    SDL_Quit();
}
