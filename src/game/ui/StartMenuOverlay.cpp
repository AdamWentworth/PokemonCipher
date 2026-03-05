#include "game/ui/StartMenuOverlay.h"

#include <algorithm>
#include <utility>

#include <SDL3/SDL_render.h>

namespace {
constexpr float kPadding = 6.0f;
constexpr float kMenuWidth = 84.0f;
constexpr float kMenuHeight = 66.0f;
constexpr float kLineHeight = 10.0f;
} // namespace

void StartMenuOverlay::open() {
    open_ = true;
    selectedIndex_ = std::clamp(selectedIndex_, 0, kEntryCount - 1);
}

void StartMenuOverlay::close() {
    open_ = false;
}

StartMenuAction StartMenuOverlay::handleKey(const SDL_Keycode key) {
    if (!open_) {
        return StartMenuAction::None;
    }

    if (key == SDLK_UP || key == SDLK_W) {
        selectedIndex_ = (selectedIndex_ + kEntryCount - 1) % kEntryCount;
        return StartMenuAction::None;
    }

    if (key == SDLK_DOWN || key == SDLK_S) {
        selectedIndex_ = (selectedIndex_ + 1) % kEntryCount;
        return StartMenuAction::None;
    }

    if (key == SDLK_ESCAPE || key == SDLK_X || key == SDLK_BACKSPACE) {
        close();
        return StartMenuAction::Closed;
    }

    if (key != SDLK_RETURN && key != SDLK_KP_ENTER && key != SDLK_Z && key != SDLK_SPACE) {
        return StartMenuAction::None;
    }

    switch (selectedIndex_) {
    case 0:
        return StartMenuAction::Party;
    case 1:
        return StartMenuAction::Bag;
    case 2:
        return StartMenuAction::Save;
    case 3:
        return StartMenuAction::Options;
    case 4:
        close();
        return StartMenuAction::Closed;
    default:
        return StartMenuAction::None;
    }
}

void StartMenuOverlay::setStatusText(std::string text) {
    statusText_ = std::move(text);
}

void StartMenuOverlay::clearStatusText() {
    statusText_.clear();
}

void StartMenuOverlay::render(SDL_Renderer* renderer, const int viewportWidth, const int viewportHeight) const {
    if (!open_ || !renderer) {
        return;
    }

    SDL_FRect menu{};
    menu.x = static_cast<float>(viewportWidth) - kMenuWidth - kPadding;
    menu.y = kPadding;
    menu.w = kMenuWidth;
    menu.h = kMenuHeight;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 20, 24, 32, 232);
    SDL_RenderFillRect(renderer, &menu);

    SDL_SetRenderDrawColor(renderer, 232, 232, 232, 255);
    SDL_RenderRect(renderer, &menu);

    for (int i = 0; i < kEntryCount; ++i) {
        const float y = menu.y + 6.0f + (static_cast<float>(i) * kLineHeight);
        if (i == selectedIndex_) {
            SDL_SetRenderDrawColor(renderer, 255, 231, 128, 255);
            SDL_RenderDebugText(renderer, menu.x + 4.0f, y, ">");
            SDL_RenderDebugText(renderer, menu.x + 12.0f, y, entryLabel(i));
        } else {
            SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
            SDL_RenderDebugText(renderer, menu.x + 12.0f, y, entryLabel(i));
        }
    }

    if (statusText_.empty()) {
        return;
    }

    SDL_FRect status{};
    status.x = kPadding;
    status.w = static_cast<float>(viewportWidth) - (kPadding * 2.0f);
    status.h = 20.0f;
    status.y = static_cast<float>(viewportHeight) - status.h - kPadding;

    SDL_SetRenderDrawColor(renderer, 20, 24, 32, 220);
    SDL_RenderFillRect(renderer, &status);
    SDL_SetRenderDrawColor(renderer, 232, 232, 232, 255);
    SDL_RenderRect(renderer, &status);
    SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
    SDL_RenderDebugText(renderer, status.x + 6.0f, status.y + 6.0f, statusText_.c_str());
}

const char* StartMenuOverlay::entryLabel(const int index) {
    switch (index) {
    case 0:
        return "POKEMON";
    case 1:
        return "BAG";
    case 2:
        return "SAVE";
    case 3:
        return "OPTIONS";
    case 4:
        return "EXIT";
    default:
        return "";
    }
}
