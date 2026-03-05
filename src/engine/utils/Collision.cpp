#include "engine/utils/Collision.h"

bool Collision::AABB(const SDL_FRect& rectA, const SDL_FRect& rectB) {
    return rectA.x + rectA.w > rectB.x &&
           rectB.x + rectB.w > rectA.x &&
           rectA.y + rectA.h > rectB.y &&
           rectB.y + rectB.h > rectA.y;
}

bool Collision::AABB(const Collider& colA, const Collider& colB) {
    return AABB(colA.rect, colB.rect);
}
