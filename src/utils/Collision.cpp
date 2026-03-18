#pragma once
#include "Collision.h"

//axis-aligned bounding box
//passing in rects
bool Collision::AABB(const SDL_FRect& rectA, const SDL_FRect& rectB) {
    //1. is the right edge of rectA >= to the left edge of rectB
    //2. is the right edge of rectB >= to the left edge of rectA
    //3. is the bottom edge of rectA>= to the top edge of rectB
    //4. is the bottom edge of rectB>= to the top edge of rectA

    if (
        rectA.x + rectA.w > rectB.x &&
        rectB.x + rectB.w > rectA.x &&
        rectA.y + rectA.h > rectB.y &&
        rectB.y + rectB.h > rectA.y
    ) {
        return true;
    }
    return false;
}; 

// passing in the colliders itself
bool Collision::AABB(const Collider& colA, const Collider& colB) {
    if (AABB(colA.rect, colB.rect)) {
        return true;
    }
    return false;
}