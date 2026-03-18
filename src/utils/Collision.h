#pragma once

#include <SDL3/SDL_rect.h>
#include "ecs/Component.h"

class Collision {
    public:
        //axis-aligned bounding box
        static bool AABB(const SDL_FRect& rectA, const SDL_FRect& rectB);
        static bool AABB(const Collider& colA, const Collider& colB);
};