#include "game/TitleScene.h"

#include <utility>

#include <SDL3/SDL_render.h>

TitleScene::TitleScene(
    TextureManager& textureManager,
    const bool hasContinueSlot,
    std::function<void(TitleMenuSelection)> onSelection
) : textureManager_(textureManager),
    hasContinueSlot_(hasContinueSlot),
    onSelection_(std::move(onSelection)) {}

void TitleScene::handleEvent(const SDL_Event& event) {
    if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) {
        return;
    }

    const SDL_Keycode key = event.key.key;
    if (key == SDLK_UP || key == SDLK_W) {
        moveSelection(-1);
        return;
    }

    if (key == SDLK_DOWN || key == SDLK_S) {
        moveSelection(1);
        return;
    }

    if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_Z || key == SDLK_SPACE) {
        confirmSelection();
    }
}

void TitleScene::update(const float dt) {
    (void)dt;
}

void TitleScene::render() {
    SDL_Renderer* renderer = textureManager_.renderer();
    if (!renderer) {
        return;
    }

    SDL_FRect titleBox{};
    titleBox.x = 30.0f;
    titleBox.y = 24.0f;
    titleBox.w = 180.0f;
    titleBox.h = 32.0f;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 18, 36, 62, 220);
    SDL_RenderFillRect(renderer, &titleBox);
    SDL_SetRenderDrawColor(renderer, 232, 232, 232, 255);
    SDL_RenderRect(renderer, &titleBox);
    SDL_SetRenderDrawColor(renderer, 255, 231, 128, 255);
    SDL_RenderDebugText(renderer, titleBox.x + 10.0f, titleBox.y + 12.0f, "POKEMON CIPHER");

    SDL_FRect menuBox{};
    menuBox.x = 58.0f;
    menuBox.y = 72.0f;
    menuBox.w = 124.0f;
    menuBox.h = 56.0f;

    SDL_SetRenderDrawColor(renderer, 20, 24, 32, 228);
    SDL_RenderFillRect(renderer, &menuBox);
    SDL_SetRenderDrawColor(renderer, 232, 232, 232, 255);
    SDL_RenderRect(renderer, &menuBox);

    for (int i = 0; i < kEntryCount; ++i) {
        const float y = menuBox.y + 8.0f + (static_cast<float>(i) * 12.0f);
        if (i == selectedIndex_) {
            SDL_SetRenderDrawColor(renderer, 255, 231, 128, 255);
            SDL_RenderDebugText(renderer, menuBox.x + 6.0f, y, ">");
            SDL_RenderDebugText(renderer, menuBox.x + 16.0f, y, entryLabel(i, hasContinueSlot_));
        } else {
            SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
            SDL_RenderDebugText(renderer, menuBox.x + 16.0f, y, entryLabel(i, hasContinueSlot_));
        }
    }

    SDL_SetRenderDrawColor(renderer, 180, 200, 220, 255);
    SDL_RenderDebugText(renderer, 8.0f, 150.0f, "Arrows/WASD + Enter");
}

void TitleScene::moveSelection(const int direction) {
    selectedIndex_ = (selectedIndex_ + kEntryCount + direction) % kEntryCount;
}

void TitleScene::confirmSelection() {
    if (!onSelection_) {
        return;
    }

    if (selectedIndex_ == 0) {
        onSelection_(TitleMenuSelection::NewGame);
        return;
    }

    if (selectedIndex_ == 1) {
        if (!hasContinueSlot_) {
            return;
        }
        onSelection_(TitleMenuSelection::ContinueGame);
        return;
    }

    onSelection_(TitleMenuSelection::DevJump);
}

const char* TitleScene::entryLabel(const int index, const bool hasContinueSlot) {
    switch (index) {
    case 0:
        return "NEW GAME";
    case 1:
        return hasContinueSlot ? "CONTINUE" : "CONTINUE (NONE)";
    case 2:
        return "DEV JUMP";
    default:
        return "";
    }
}
