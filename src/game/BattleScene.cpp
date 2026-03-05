#include "game/BattleScene.h"

#include <sstream>
#include <utility>

#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_render.h>

BattleScene::BattleScene(
    TextureManager& textureManager,
    GameState& gameState,
    WildEncounter encounter,
    std::function<void()> onBattleComplete
) : textureManager_(textureManager),
    gameState_(gameState),
    encounter_(std::move(encounter)),
    onBattleComplete_(std::move(onBattleComplete)) {
    gameState_.setFlag("in_battle", true);
    gameState_.setVar("last_wild_species_id", encounter_.speciesId);
    gameState_.setVar("last_wild_level", encounter_.level);
}

void BattleScene::handleEvent(const SDL_Event& event) {
    if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) {
        return;
    }

    const SDL_Keycode key = event.key.key;
    if (phase_ == Phase::CommandSelect) {
        if (key == SDLK_UP || key == SDLK_W) {
            moveSelection(-1);
            return;
        }

        if (key == SDLK_DOWN || key == SDLK_S) {
            moveSelection(1);
            return;
        }
    }

    if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE || key == SDLK_Z || key == SDLK_X) {
        confirmSelection();
    }
}

void BattleScene::update(const float dt) {
    (void)dt;
}

void BattleScene::render() {
    SDL_Renderer* renderer = textureManager_.renderer();
    if (!renderer) {
        return;
    }

    SDL_FRect topBox{};
    topBox.x = 8.0f;
    topBox.y = 8.0f;
    topBox.w = 224.0f;
    topBox.h = 52.0f;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 18, 24, 30, 232);
    SDL_RenderFillRect(renderer, &topBox);
    SDL_SetRenderDrawColor(renderer, 232, 232, 232, 255);
    SDL_RenderRect(renderer, &topBox);

    std::ostringstream encounterLine;
    encounterLine << "WILD " << encounter_.speciesName << "  Lv" << encounter_.level;
    SDL_SetRenderDrawColor(renderer, 255, 231, 128, 255);
    SDL_RenderDebugText(renderer, topBox.x + 8.0f, topBox.y + 8.0f, encounterLine.str().c_str());

    SDL_SetRenderDrawColor(renderer, 195, 210, 225, 255);
    SDL_RenderDebugText(renderer, topBox.x + 8.0f, topBox.y + 24.0f, "Battle prototype scene");

    SDL_FRect commandBox{};
    commandBox.x = 8.0f;
    commandBox.y = 110.0f;
    commandBox.w = 224.0f;
    commandBox.h = 42.0f;

    SDL_SetRenderDrawColor(renderer, 20, 20, 24, 232);
    SDL_RenderFillRect(renderer, &commandBox);
    SDL_SetRenderDrawColor(renderer, 232, 232, 232, 255);
    SDL_RenderRect(renderer, &commandBox);

    if (phase_ == Phase::CommandSelect) {
        for (int i = 0; i < kActionCount; ++i) {
            const float lineY = commandBox.y + 8.0f + (12.0f * static_cast<float>(i));
            if (i == selectedAction_) {
                SDL_SetRenderDrawColor(renderer, 255, 231, 128, 255);
                SDL_RenderDebugText(renderer, commandBox.x + 8.0f, lineY, ">");
                SDL_RenderDebugText(renderer, commandBox.x + 18.0f, lineY, actionLabel(i));
            } else {
                SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
                SDL_RenderDebugText(renderer, commandBox.x + 18.0f, lineY, actionLabel(i));
            }
        }
    } else {
        SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
        SDL_RenderDebugText(renderer, commandBox.x + 8.0f, commandBox.y + 8.0f, resultText_.c_str());
        SDL_RenderDebugText(renderer, commandBox.x + 8.0f, commandBox.y + 20.0f, "Press confirm to return.");
    }
}

void BattleScene::moveSelection(const int direction) {
    selectedAction_ = (selectedAction_ + kActionCount + direction) % kActionCount;
}

void BattleScene::confirmSelection() {
    if (phase_ == Phase::Result) {
        gameState_.setFlag("in_battle", false);
        if (onBattleComplete_) {
            onBattleComplete_();
        }
        return;
    }

    if (selectedAction_ == 0) {
        resultText_ = "You defeated the wild foe.";
        phase_ = Phase::Result;
        return;
    }

    resultText_ = "Got away safely.";
    phase_ = Phase::Result;
}

const char* BattleScene::actionLabel(const int index) const {
    switch (index) {
    case 0:
        return "FIGHT (placeholder)";
    case 1:
        return "RUN";
    default:
        return "";
    }
}
