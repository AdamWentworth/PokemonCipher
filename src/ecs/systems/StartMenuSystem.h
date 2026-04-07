#pragma once

#include <SDL3/SDL.h>

class StartMenuSystem {
public:
    void setViewportSize(const int width, const int height) {
        viewportWidth = width;
        viewportHeight = height;
    }

    bool isOpen() const {
        return open;
    }

    void openMenu() {
        open = true;
    }

    void close() {
        open = false;
    }

    // This keeps all of the start menu button rules in one place, so Scene
    // only has to ask whether the menu changed or returned a chosen entry.
    const char* handleKey(const SDL_Keycode key) {
        if (!open) {
            return nullptr;
        }

        if (key == SDLK_UP || key == SDLK_W) {
            selectedIndex = (selectedIndex + kEntryCount - 1) % kEntryCount;
            return nullptr;
        }

        if (key == SDLK_DOWN || key == SDLK_S) {
            selectedIndex = (selectedIndex + 1) % kEntryCount;
            return nullptr;
        }

        if (key == SDLK_ESCAPE || key == SDLK_X || key == SDLK_BACKSPACE) {
            close();
            return nullptr;
        }

        if (key != SDLK_RETURN && key != SDLK_KP_ENTER && key != SDLK_SPACE && key != SDLK_Z) {
            return nullptr;
        }

        const int chosenIndex = selectedIndex;
        close();
        if (chosenIndex == kExitEntry) {
            return nullptr;
        }

        return entryLabel(chosenIndex);
    }

    void render(SDL_Renderer* renderer) const {
        if (!open || !renderer) {
            return;
        }

        constexpr float kPadding = 6.0f;
        constexpr float kTopOffset = 30.0f;
        constexpr float kMenuWidth = 96.0f;
        constexpr float kMenuHeight = 74.0f;
        constexpr float kLineHeight = 13.0f;
        constexpr float kTextTop = 8.0f;

        SDL_FRect menu{};
        // Keeping the box on the right still feels like a Pokemon start menu,
        // while moving it farther down and sizing it around the entries makes
        // it easier to read without leaving a big empty gap under EXIT.
        menu.x = static_cast<float>(viewportWidth) - kMenuWidth - kPadding;
        menu.y = kPadding + kTopOffset;
        menu.w = kMenuWidth;
        menu.h = kMenuHeight;

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 20, 24, 32, 232);
        SDL_RenderFillRect(renderer, &menu);
        SDL_SetRenderDrawColor(renderer, 232, 232, 232, 255);
        SDL_RenderRect(renderer, &menu);

        for (int i = 0; i < kEntryCount; ++i) {
            const float y = menu.y + kTextTop + (static_cast<float>(i) * kLineHeight);
            if (i == selectedIndex) {
                SDL_SetRenderDrawColor(renderer, 255, 231, 128, 255);
                SDL_RenderDebugText(renderer, menu.x + 4.0f, y, ">");
                SDL_RenderDebugText(renderer, menu.x + 12.0f, y, entryLabel(i));
            } else {
                SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
                SDL_RenderDebugText(renderer, menu.x + 12.0f, y, entryLabel(i));
            }
        }
    }

private:
    static constexpr int kEntryCount = 5;
    static constexpr int kExitEntry = 4;

    bool open = false;
    int viewportWidth = 0;
    int viewportHeight = 0;
    int selectedIndex = 0;

    static const char* entryLabel(const int index) {
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
};
