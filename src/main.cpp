#include <SDL3/SDL.h>

#include <iostream>
#include <memory>
#include <optional>
#include <string>

#include "app/LaunchOptions.h"
#include "engine/Game.h"
#include "engine/utils/Vector2D.h"
#include "game/OverworldScene.h"
#include "game/TitleScene.h"
#include "game/save/SaveGameStorage.h"
#include "game/state/GameState.h"
#include "game/world/DefaultMapRegistry.h"
#include "game/world/MapRegistry.h"

namespace {
constexpr int kPrimarySaveSlot = 1;

enum class PendingAction {
    None,
    StartNewGame,
    ContinueGame,
    StartDevJump
};

struct OverworldStartConfig {
    std::string mapId = "pallet_town";
    std::string spawnId = "default";
    std::optional<Vector2D> pixelOverride = std::nullopt;
};

void applyNewGameState(GameState& gameState) {
    gameState.setFlag("intro_complete", false);
    gameState.setFlag("starter_eevee_obtained", false);
    gameState.setVar("story_checkpoint", 0);
    gameState.clearParty();
}

void applySaveDataToState(GameState& gameState, const SaveSlotData& saveData) {
    gameState.setFlag("intro_complete", saveData.introComplete);
    gameState.setFlag("starter_eevee_obtained", saveData.starterEeveeObtained);
    gameState.setVar("story_checkpoint", saveData.storyCheckpoint);
    gameState.clearParty();
    for (const PartyPokemon& member : saveData.party) {
        gameState.addPartyPokemon(member.speciesId, member.level, member.isPartner);
    }
}
} // namespace

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
    SaveGameStorage saveStorage;
    OverworldStartConfig overworldStart{};

    overworldStart.mapId = bootOptions.mapId;
    overworldStart.spawnId = bootOptions.spawnId;

    MapRegistry mapRegistry = buildDefaultMapRegistry();
    PendingAction pendingAction = PendingAction::None;

    scenes.registerScene("title", [&textures, &saveStorage, &pendingAction]() {
        return std::make_unique<TitleScene>(
            textures,
            saveStorage.hasSlot(kPrimarySaveSlot),
            [&pendingAction](const TitleMenuSelection selection) {
                switch (selection) {
                case TitleMenuSelection::NewGame:
                    pendingAction = PendingAction::StartNewGame;
                    break;
                case TitleMenuSelection::ContinueGame:
                    pendingAction = PendingAction::ContinueGame;
                    break;
                case TitleMenuSelection::DevJump:
                    pendingAction = PendingAction::StartDevJump;
                    break;
                }
            }
        );
    });

    scenes.registerScene("overworld", [&textures, &gameState, &mapRegistry, logicalWidth, logicalHeight, &overworldStart, &saveStorage]() {
        auto scene = std::make_unique<OverworldScene>(
            textures,
            gameState,
            mapRegistry,
            overworldStart.mapId,
            [&saveStorage, &gameState](const std::string& mapId, const Vector2D& playerPosition) {
                SaveSlotData saveData{};
                saveData.mapId = mapId;
                saveData.playerX = playerPosition.x;
                saveData.playerY = playerPosition.y;
                saveData.introComplete = gameState.getFlag("intro_complete");
                saveData.starterEeveeObtained = gameState.getFlag("starter_eevee_obtained");
                saveData.storyCheckpoint = gameState.getVar("story_checkpoint");
                saveData.party = gameState.party();
                return saveStorage.writeSlot(kPrimarySaveSlot, saveData);
            },
            logicalWidth,
            logicalHeight
        );

        if (overworldStart.spawnId != "default") {
            scene->warpTo(overworldStart.mapId, overworldStart.spawnId);
        }

        if (overworldStart.pixelOverride.has_value()) {
            scene->teleportPlayer(*overworldStart.pixelOverride);
        }

        return scene;
    });

    if (bootOptions.mode == BootMode::Overworld) {
        gameState.setFlag("intro_complete", true);
        if (!scenes.changeScene("overworld")) {
            return 1;
        }
    } else if (bootOptions.mode == BootMode::Intro) {
        applyNewGameState(gameState);
        if (!scenes.changeScene("overworld")) {
            return 1;
        }
    } else if (!scenes.changeScene("title")) {
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

        if (pendingAction != PendingAction::None) {
            const PendingAction action = pendingAction;
            pendingAction = PendingAction::None;

            if (action == PendingAction::StartNewGame) {
                applyNewGameState(gameState);
                overworldStart.mapId = "pallet_town";
                overworldStart.spawnId = "default";
                overworldStart.pixelOverride = std::nullopt;
                if (!scenes.changeScene("overworld")) {
                    return 1;
                }
            } else if (action == PendingAction::ContinueGame) {
                SaveSlotData saveData{};
                if (!saveStorage.loadSlot(kPrimarySaveSlot, saveData)) {
                    std::cout << "Continue requested but save slot could not be loaded.\n";
                    continue;
                }

                applySaveDataToState(gameState, saveData);
                overworldStart.mapId = saveData.mapId;
                overworldStart.spawnId = "default";
                overworldStart.pixelOverride = Vector2D(saveData.playerX, saveData.playerY);
                if (!scenes.changeScene("overworld")) {
                    return 1;
                }
            } else if (action == PendingAction::StartDevJump) {
                gameState.setFlag("intro_complete", true);
                overworldStart.mapId = bootOptions.mapId;
                overworldStart.spawnId = bootOptions.spawnId;
                overworldStart.pixelOverride = std::nullopt;
                if (!scenes.changeScene("overworld")) {
                    return 1;
                }
            }
        }

        game.render();

        const int frameTimeMs = static_cast<int>(SDL_GetTicks() - frameStart);
        if (frameTimeMs < desiredFrameTimeMs) {
            SDL_Delay(desiredFrameTimeMs - frameTimeMs);
        }
    }

    return 0;
}
