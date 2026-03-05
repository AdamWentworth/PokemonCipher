#include <SDL3/SDL.h>

#include <memory>

#include "app/LaunchOptions.h"
#include "engine/Game.h"
#include "game/OverworldScene.h"
#include "game/state/GameState.h"
#include "game/world/DefaultMapRegistry.h"
#include "game/world/MapRegistry.h"

int main(int argc, char** argv) {
    const LaunchOptions bootOptions = parseLaunchOptions(argc, argv);
    if (!bootOptions.valid) {
        printLaunchUsage();
        return 1;
    }

    if (bootOptions.showHelp) {
        printLaunchUsage();
        return 0;
    }

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
    GameState gameState;

    if (bootOptions.mode == BootMode::Overworld) {
        gameState.setFlag("intro_complete", true);
    } else if (bootOptions.mode == BootMode::Intro) {
        gameState.setFlag("intro_complete", false);
        gameState.setFlag("starter_eevee_obtained", false);
        gameState.setVar("story_checkpoint", 0);
        gameState.clearParty();
    }

    MapRegistry mapRegistry = buildDefaultMapRegistry();

    scenes.registerScene("overworld", [&textures, &gameState, &mapRegistry, logicalWidth, logicalHeight, bootOptions]() {
        auto scene = std::make_unique<OverworldScene>(
            textures,
            gameState,
            mapRegistry,
            bootOptions.mapId,
            logicalWidth,
            logicalHeight
        );

        if (bootOptions.spawnId != "default") {
            scene->warpTo(bootOptions.mapId, bootOptions.spawnId);
        }

        return scene;
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
