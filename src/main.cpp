#include <iostream>
#include "Game.h"
#include "utils/Vector2D.h"

// Global variable
Game *game = nullptr;

// void Vector2D_DemoOperators();

int main(int argc, char** argv) {
    // Vector2D_DemoOperators();
    // int actualFrameRate{};
    float deltaTime{};

    game = new Game();
    game->init("Pokemon Cipher", 800, 600, false);

    Uint64 ticks = SDL_GetTicks();

    // Game loop
    while (game->running()) {

        Uint64 frameStart = SDL_GetTicks();

        Uint64 currentTicks = frameStart;
        deltaTime = (currentTicks - ticks) / 1000.0f;
        ticks = currentTicks;

        const int FPS = 60; // 60 is the closest refresh rate of most our monitors, 30 (half of the work)
        const int desiredFrameTime = 1000 / FPS; // 16ms per frame

        // Uint64 ticks;
        // int actualFrameTime{};

        game -> handleEvents();
        game -> update(deltaTime);
        game -> render();

        int actualFrameTime = static_cast<int>(SDL_GetTicks() - frameStart);

        if (actualFrameTime < desiredFrameTime) {
            SDL_Delay(desiredFrameTime - actualFrameTime);
        }
    }

    delete game;

    return 0;
}
