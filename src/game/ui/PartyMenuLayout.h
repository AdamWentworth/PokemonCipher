#pragma once

#include <array>
#include <string>

#include <SDL3/SDL_rect.h>

struct PartyMenuLayout {
    float designWidth = 240.0f;
    float designHeight = 160.0f;
    float textBaselineYOffset = 2.0f;

    std::array<SDL_FRect, 6> slotRects = {
        SDL_FRect{8.0f, 24.0f, 80.0f, 56.0f},
        SDL_FRect{96.0f, 8.0f, 136.0f, 24.0f},
        SDL_FRect{96.0f, 32.0f, 136.0f, 24.0f},
        SDL_FRect{96.0f, 56.0f, 136.0f, 24.0f},
        SDL_FRect{96.0f, 80.0f, 136.0f, 24.0f},
        SDL_FRect{96.0f, 104.0f, 136.0f, 24.0f},
    };

    std::array<SDL_FPoint, 6> pokeballPositions = {
        SDL_FPoint{16.0f, 34.0f},
        SDL_FPoint{102.0f, 25.0f},
        SDL_FPoint{102.0f, 49.0f},
        SDL_FPoint{102.0f, 73.0f},
        SDL_FPoint{102.0f, 97.0f},
        SDL_FPoint{102.0f, 121.0f},
    };

    std::array<SDL_FPoint, 6> pokemonIconPositions = {
        SDL_FPoint{16.0f, 40.0f},
        SDL_FPoint{104.0f, 18.0f},
        SDL_FPoint{104.0f, 42.0f},
        SDL_FPoint{104.0f, 66.0f},
        SDL_FPoint{104.0f, 90.0f},
        SDL_FPoint{104.0f, 114.0f},
    };

    SDL_FRect messageRect{8.0f, 128.0f, 160.0f, 24.0f};
    SDL_FRect cancelRect{176.0f, 128.0f, 56.0f, 24.0f};
    SDL_FRect commandRect{176.0f, 82.0f, 56.0f, 42.0f};

    float nameTextOffsetX = 22.0f;
    float nameTextMainYOffset = 8.0f;
    float nameTextOtherYOffset = 3.0f;

    float levelTextMainX = 32.0f;
    float levelTextOtherX = 52.0f;
    float levelTextMainYOffset = 10.0f;
    float levelTextOtherYOffset = 0.0f;

    float hpBarMainX = 24.0f;
    float hpBarOtherX = 88.0f;
    float hpBarMainY = 36.0f;
    float hpBarOtherY = 11.0f;
    float hpBarWidth = 48.0f;
    float hpBarHeight = 3.0f;

    float hpTextMainX = 38.0f;
    float hpTextOtherX = 100.0f;
    float hpTextMainY = 42.0f;
    float hpTextOtherY = 13.0f;

    float promptTextXOffset = 6.0f;
    float promptTextYOffset = 8.0f;

    float commandArrowXOffset = 4.0f;
    float commandLabelXOffset = 12.0f;
    float commandLineStartYOffset = 6.0f;
    float commandLineStep = 10.0f;
};

PartyMenuLayout makeDefaultPartyMenuLayout();
bool loadPartyMenuLayoutFromLuaFile(const std::string& scriptPath, PartyMenuLayout& outLayout, std::string& errorOut);

