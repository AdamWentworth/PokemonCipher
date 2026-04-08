#include "Game.h"

#include <iostream>

bool Game::initializeRuntime(
    const char* title,
    const int width,
    const int height,
    const bool fullscreen,
    const int gameViewWidth,
    const int gameViewHeight
) {
    int flags = 0;
    if (fullscreen) {
        flags = SDL_WINDOW_FULLSCREEN;
    }

    // SDL/window/renderer startup changes for engine reasons, so it lives in
    // its own file instead of crowding the higher-level game loop code.
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "SDL video init failed: " << SDL_GetError() << std::endl;
        isRunning = false;
        return false;
    }

    std::cout << "Subsystem initialized..." << std::endl;

    window = SDL_CreateWindow(title, width, height, static_cast<SDL_WindowFlags>(flags));
    if (!window) {
        std::cout << "Window could not be created..." << std::endl;
        std::cout << "SDL error: " << SDL_GetError() << std::endl;
        isRunning = false;
        return false;
    }

    std::cout << "Window created..." << std::endl;

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cout << "Renderer could not be created..." << std::endl;
        std::cout << "SDL error: " << SDL_GetError() << std::endl;
        isRunning = false;
        return false;
    }

    std::cout << "Renderer created..." << std::endl;

    // Tell SDL to draw into that smaller view and scale it up to the real
    // window. Because this view is exactly half the window size, it fills
    // the screen neatly while showing less of the map at once.
    if (!SDL_SetRenderLogicalPresentation(renderer, gameViewWidth, gameViewHeight, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE)) {
        std::cout << "Failed to set game view..." << std::endl;
        std::cout << "SDL error: " << SDL_GetError() << std::endl;
        isRunning = false;
        return false;
    }

    isRunning = true;
    return true;
}
