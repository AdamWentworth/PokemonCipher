#include "Game.h"
#include "TextureManager.h"

#include <ctime>

// constructor
Game::Game() {}


// deconstructor
Game::~Game() {
    destroy();
}

void Game::init(const char* title, int width, int height, bool fullscreen) {
    // For random
    srand(static_cast<unsigned>(time(nullptr)));

    // Draw the game into a view that is half the size of the real window, then
    // let SDL scale it back up. This makes the camera feel more zoomed in
    // while still filling the whole game window cleanly.
    const int gameViewWidth = width / 2;
    const int gameViewHeight = height / 2;

    // Game.cpp now coordinates startup at a high level, while lower-level
    // runtime setup and content registration live in their own files.
    if (!initializeRuntime(title, width, height, fullscreen, gameViewWidth, gameViewHeight)) {
        return;
    }

    loadGameContent(gameViewWidth, gameViewHeight);
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
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Every frame the renderer is cleared with the draw color
    SDL_RenderClear(renderer);

    sceneManager.render();

    SDL_RenderPresent(renderer);
}

void Game::destroy() {
    TextureManager::clean();
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "GameDestroyed" << std::endl;
}
