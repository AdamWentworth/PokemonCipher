#pragma once

#include <string>
#include <SDL3/SDL.h>

#include "ecs/systems/InteractionSystem.h"

class DialogueSystem {
public:
    void setViewportSize(const int width, const int height) {
        viewportWidth = width;
        viewportHeight = height;
    }

    bool isOpen() const {
        return open;
    }

    void close() {
        open = false;
        speaker.clear();
        text.clear();
    }

    // A shared open helper keeps Oak dialogue and encounter popups using the
    // same box instead of growing separate UI code for each message type.
    void openMessage(const std::string& newSpeaker, const std::string& newText) {
        open = true;
        speaker = newSpeaker;
        text = newText;
    }

    // This first pass only needs enough data to prove the interaction-to-text
    // flow, so we open a line for Oak and leave other ids as future work.
    bool openForInteraction(const InteractionPoint& interaction) {
        if (interaction.id == "oak_lab_eevee") {
            openMessage("PROF. OAK", "That EEVEE is one of a kind.");
            return true;
        }

        return false;
    }

    void render(SDL_Renderer* renderer) const {
        if (!open || !renderer) {
            return;
        }

        constexpr float kPadding = 6.0f;
        constexpr float kBoxHeight = 46.0f;
        SDL_FRect box{};
        box.x = kPadding;
        box.w = static_cast<float>(viewportWidth) - (kPadding * 2.0f);
        box.h = kBoxHeight;
        box.y = static_cast<float>(viewportHeight) - box.h - kPadding;

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        // This matches the darker menu-style panel look from Pokemon2 so the
        // dialogue box feels like part of the same Pokemon UI family.
        SDL_SetRenderDrawColor(renderer, 20, 24, 32, 230);
        SDL_RenderFillRect(renderer, &box);
        SDL_SetRenderDrawColor(renderer, 248, 248, 248, 255);
        SDL_RenderRect(renderer, &box);

        if (!speaker.empty()) {
            SDL_SetRenderDrawColor(renderer, 255, 231, 128, 255);
            SDL_RenderDebugText(renderer, box.x + 6.0f, box.y + 6.0f, speaker.c_str());
        }

        SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
        SDL_RenderDebugText(renderer, box.x + 6.0f, box.y + 18.0f, text.c_str());

        SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
        SDL_RenderDebugText(renderer, box.x + box.w - 10.0f, box.y + box.h - 10.0f, ">");
    }

private:
    bool open = false;
    int viewportWidth = 0;
    int viewportHeight = 0;
    std::string speaker;
    std::string text;
};
