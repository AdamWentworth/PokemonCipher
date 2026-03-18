#include "Game.h"
#include "Map.h"
#include "TextureManager.h"
#include "manager/AssetManager.h"

#include <iostream>
#include <ostream>
#include <random>
#include <ctime>   // time()

#include "ecs/Component.h"

Map* map = nullptr;

std::function<void(std::string)> Game::onSceneChangeRequest;

// constructor
Game::Game() {}


// deconstructor
Game::~Game() {
    destroy();
}

void Game::init(const char* title, int width, int height, bool fullscreen) {

    // For random
    srand(static_cast<unsigned>(time(nullptr)));

    int flags = 0;
    if (fullscreen) {
        flags = SDL_WINDOW_FULLSCREEN;
    }

    // init SDL library
    // SDL3: SDL_Init / SDL_InitSubSystem returns bool (true=success, false=failure)
    if (SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "Subsystem initialized..." << std::endl;

        window = SDL_CreateWindow(title, width, height, static_cast<SDL_WindowFlags>(flags));
        if (window) {
            std::cout << "Window created..." << std::endl;
        } else {
            std::cout << "Window could not be created..." << std::endl;
            std::cout << "SDL error: " << SDL_GetError() << std::endl;
            isRunning = false;
            return;
        }

        // creating rendering context
        // windows(direct3d(directx)) mac(Metal)
        renderer = SDL_CreateRenderer(window, nullptr);

        if (renderer) {
            std::cout << "Renderer created..." << std::endl;
        } else {
            std::cout << "Renderer could not be created..." << std::endl;
            std::cout << "SDL error: " << SDL_GetError() << std::endl;
            isRunning = false;
            return;
        }

        isRunning = true;
    } else {
        std::cout << "SDL video init failed: " << SDL_GetError() << std::endl;
        isRunning = false;
        return;
    }

    AssetManager::loadAnimation("player", "assets/animations/lamb_animations.xml");
    AssetManager::loadAnimation("enemy", "assets/animations/bird_animations.xml");

    sceneManager.load("level1", "assets/map.tmx", width, height);
    sceneManager.load("level2", "assets/map2.tmx", width, height);

    sceneManager.changeSceneDeferred("level1");

    onSceneChangeRequest = [this](std::string sceneName) {
        if (sceneManager.currentScene->getName() == "level2" && sceneName == "level2") {
            std::cout << "You win!" << std::endl;
            isRunning = false;
            return;
        }
        if (sceneName == "gameover") {
            std::cout << "Game over!" << std::endl;
            isRunning = false;
            return;
        }

        sceneManager.changeSceneDeferred(sceneName);
    };


}

void Game::handleEvents() {

    // SDL listens to the OS for input events internally and it addes them to a queue
    // SDL_Event event;
    // Check for next event, if an event is available it will remove from the queue and store in event
    SDL_PollEvent(&event);

    switch (event.type) {
        case SDL_EVENT_QUIT:
            isRunning = false;
            break;
        default:
            break;
    }

}


void Game::update(float dt) {
    sceneManager.update(dt, event);
}

void Game::render() {

    // randomizeColor();
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Every frame the renderer is cleared with the draw color
    SDL_RenderClear(renderer);

    sceneManager.render();

    SDL_RenderPresent(renderer);
}

void Game::randomizeColor() {
    static Uint32 lastTime = 0;
    Uint32 currentTime = SDL_GetTicks();

    if (currentTime - lastTime >= 1000) {
        // r = dist([&]gen);
        // g = dist([&]gen);
        // b = dist([&]gen);

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
