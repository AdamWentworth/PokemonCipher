#pragma once

#include <SDL3/SDL.h>

#include "manager/SceneManager.h"

class Game {
public:
    Game(); // constructor
    ~Game();

    // theses are types that SDL uses
    // (Simple DirectMedia Layer: communicate with OS/hardware <-> input)
    void init(const char* title, int width, int height, bool fullscreen);

    // game loop functions
    void handleEvents(); // checks for input and system events
    void update(float deltaTime); // handles the game logic and changes to game state
    void render(); // handles the drawing of the current game state to the screen

    // used to free resources
    void destroy();

    bool running() {
        return isRunning;
    }

    SDL_Renderer* renderer = nullptr;

    SceneManager sceneManager;

private:
    bool isRunning = false;

    SDL_Window* window = nullptr;
    SDL_Event event;
    bool initializeRuntime(const char* title, int width, int height, bool fullscreen, int gameViewWidth, int gameViewHeight);
    void loadGameContent(int gameViewWidth, int gameViewHeight);
};
