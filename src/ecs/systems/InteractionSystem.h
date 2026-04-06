#pragma once

#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "utils/Collision.h"
#include "utils/Vector2D.h"

struct InteractionPoint {
    SDL_FRect rect{};
    std::string id;
};

class InteractionSystem {
public:
    // This checks the tile in front of the player because Pokemon-style
    // interactions happen one tile ahead, not under the player's feet.
    const InteractionPoint* findInteraction(
        const SDL_FRect& playerRect,
        const Vector2D& facingDirection,
        const float tileSize,
        const std::vector<InteractionPoint>& interactions
    ) const {
        SDL_FRect probe = playerRect;
        probe.x += facingDirection.x * tileSize;
        probe.y += facingDirection.y * tileSize;

        for (const auto& interaction : interactions) {
            if (Collision::AABB(probe, interaction.rect)) {
                return &interaction;
            }
        }

        return nullptr;
    }
};
