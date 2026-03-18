#include "Game.h"

#include <ctime>
#include <iostream>
#include <ostream>
#include <random>

#include "TextureManager.h"
#include "manager/AssetManager.h"

std::function<void(std::string)> Game::onSceneChangeRequest;

Game::Game() {}

Game::~Game() {
    destroy();
}

void Game::init(const char* title, int width, int height, bool fullscreen) {
    srand(static_cast<unsigned>(time(nullptr)));

    int flags = 0;
    if (fullscreen) {
        flags = SDL_WINDOW_FULLSCREEN;
    }

    if (SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "Subsystem initialized..." << std::endl;

        window = SDL_CreateWindow(title, width, height, static_cast<SDL_WindowFlags>(flags));
        if (!window) {
            std::cout << "Window could not be created..." << std::endl;
            std::cout << "SDL error: " << SDL_GetError() << std::endl;
            isRunning = false;
            return;
        }

        renderer = SDL_CreateRenderer(window, nullptr);
        if (!renderer) {
            std::cout << "Renderer could not be created..." << std::endl;
            std::cout << "SDL error: " << SDL_GetError() << std::endl;
            isRunning = false;
            return;
        }

        std::cout << "Window created..." << std::endl;
        std::cout << "Renderer created..." << std::endl;
        isRunning = true;
    } else {
        std::cout << "SDL video init failed: " << SDL_GetError() << std::endl;
        isRunning = false;
        return;
    }

    AssetManager::loadAnimation("player", "assets/animations/wes_overworld.xml");

    sceneManager.load("overworld", "assets/maps/pallet_town/pallet_town_map.tmx", width, height);
    sceneManager.changeSceneDeferred("overworld");

    onSceneChangeRequest = [this](std::string sceneName) {
        sceneManager.changeSceneDeferred(sceneName);
    };
}

void Game::handleEvents() {
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            isRunning = false;
        }
    }
}

void Game::update(float dt) {
    sceneManager.update(dt, event);
}

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    sceneManager.render();
    SDL_RenderPresent(renderer);
}

void Game::randomizeColor() {
    static Uint32 lastTime = 0;
    Uint32 currentTime = SDL_GetTicks();

    if (currentTime - lastTime >= 1000) {
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        lastTime = currentTime;
    }
}

void Game::destroy() {
    TextureManager::clean();
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "GameDestroyed" << std::endl;
}
