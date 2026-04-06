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

        // Tell SDL to draw into that smaller view and scale it up to the real
        // window. Because this view is exactly half the window size, it fills
        // the screen neatly while showing less of the map at once.
        if (!SDL_SetRenderLogicalPresentation(renderer, gameViewWidth, gameViewHeight, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE)) {
            std::cout << "Failed to set game view..." << std::endl;
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

    AssetManager::loadAnimation("player", "assets/animations/wes_overworld.xml");

    // These scene names match the map names already written into the TMX warp
    // data, so doors and route exits can switch maps without extra remapping code.
    sceneManager.load("MAP_PALLET_TOWN", "assets/maps/pallet_town/pallet_town_map.tmx", "assets/tilesets/pallet_town/pallet_town_tileset.png", gameViewWidth, gameViewHeight);
    sceneManager.load("MAP_PALLET_TOWN_PLAYERS_HOUSE_1F", "assets/maps/pallet_town_players_house_1f/pallet_town_players_house_1f_map.tmx", "assets/tilesets/pallet_town_players_house_1f/pallet_town_players_house_1f_tileset.png", gameViewWidth, gameViewHeight);
    sceneManager.load("MAP_PALLET_TOWN_PLAYERS_HOUSE_2F", "assets/maps/pallet_town_players_house_2f/pallet_town_players_house_2f_map.tmx", "assets/tilesets/pallet_town_players_house_2f/pallet_town_players_house_2f_tileset.png", gameViewWidth, gameViewHeight);
    sceneManager.load("MAP_PALLET_TOWN_RIVALS_HOUSE", "assets/maps/pallet_town_rivals_house/pallet_town_rivals_house_map.tmx", "assets/tilesets/pallet_town_rivals_house/pallet_town_rivals_house_tileset.png", gameViewWidth, gameViewHeight);
    sceneManager.load("MAP_PALLET_TOWN_PROFESSOR_OAKS_LAB", "assets/maps/pallet_town_professor_oaks_lab/pallet_town_professor_oaks_lab_map.tmx", "assets/tilesets/pallet_town_professor_oaks_lab/pallet_town_professor_oaks_lab_tileset.png", gameViewWidth, gameViewHeight);
    sceneManager.load("MAP_ROUTE1", "assets/maps/route_1/route_1_map.tmx", "assets/tilesets/route_1/route_1_tileset.png", gameViewWidth, gameViewHeight);
    sceneManager.changeSceneDeferred("MAP_PALLET_TOWN");
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
