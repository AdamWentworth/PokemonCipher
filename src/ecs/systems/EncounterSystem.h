#pragma once

#include <cstdlib>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "utils/Collision.h"

struct EncounterZone {
    SDL_FRect rect{};
    std::string tableId;
};

class EncounterSystem {
public:
    // This first pass only needs one simple rule: when the player lands in
    // grass, give that tile a small random chance to start an encounter.
    const char* tryStartEncounter(
        const SDL_FRect& playerRect,
        const std::vector<EncounterZone>& encounterZones
    ) const {
        const EncounterZone* encounterZone = findTouchedEncounter(playerRect, encounterZones);
        if (!encounterZone) {
            return nullptr;
        }

        // A low roll chance keeps grass from firing every single step while
        // still making Route 1 feel like it can surprise the player.
        if ((std::rand() % 8) != 0) {
            return nullptr;
        }

        return textForTable(encounterZone->tableId);
    }

private:
    static const EncounterZone* findTouchedEncounter(
        const SDL_FRect& playerRect,
        const std::vector<EncounterZone>& encounterZones
    ) {
        for (const auto& encounterZone : encounterZones) {
            if (Collision::AABB(playerRect, encounterZone.rect)) {
                return &encounterZone;
            }
        }

        return nullptr;
    }

    static const char* textForTable(const std::string& tableId) {
        // Keeping the table id from TMX lets later routes point at different
        // encounter sets without rewriting how grass is loaded.
        if (tableId == "route_1_grass") {
            return "A wild POKEMON appeared!";
        }

        return "A wild POKEMON appeared!";
    }
};
