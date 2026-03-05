#include <SDL3/SDL.h>

#include <memory>

#include "engine/Game.h"
#include "game/OverworldScene.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    constexpr int windowWidth = 960;
    constexpr int windowHeight = 640;
    constexpr int logicalWidth = 240;
    constexpr int logicalHeight = 160;

    Game game;
    if (!game.init("PokemonCipher", windowWidth, windowHeight, false)) {
        return 1;
    }

    if (!game.setLogicalPresentation(logicalWidth, logicalHeight, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE)) {
        return 1;
    }

    auto& scenes = game.sceneManager();
    auto& textures = game.textureManager();

    scenes.registerScene("overworld", [&textures, logicalWidth, logicalHeight]() {
        return std::make_unique<OverworldScene>(
            textures,
            "assets/maps/pallet_town.tmx",
            "assets/tilesets/pallet_town_metatiles.png",
            "assets/tilesets/pallet_town_cover_metatiles.png",
            logicalWidth,
            logicalHeight
        );
    });

    if (!scenes.changeScene("overworld")) {
        return 1;
    }

    Uint64 previousTicks = SDL_GetTicks();
    constexpr int targetFps = 60;
    constexpr int desiredFrameTimeMs = 1000 / targetFps;

    while (game.running()) {
        const Uint64 frameStart = SDL_GetTicks();
        const Uint64 currentTicks = frameStart;
        const float deltaTime = static_cast<float>(currentTicks - previousTicks) / 1000.0f;
        previousTicks = currentTicks;

        game.handleEvents();
        game.update(deltaTime);
        game.render();

        const int frameTimeMs = static_cast<int>(SDL_GetTicks() - frameStart);
        if (frameTimeMs < desiredFrameTimeMs) {
            SDL_Delay(desiredFrameTimeMs - frameTimeMs);
        }
    }

    return 0;
}
