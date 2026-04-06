#pragma once
#include <SDL3/SDL_render.h>
#include "utils/Vector2D.h"
#include "systems/AnimationClip.h"
#include <functional>
#include <string>
#include <unordered_map>

struct Transform {
    Vector2D position;
    float rotation{};
    float scale{};
    Vector2D oldPosition{};
};

struct GridMovement {
    // This keeps everything the player needs for one-tile movement.
    float tileSize = 32.0f;
    // How fast the player moves toward the next tile.
    float speed = 120.0f;
    // The exact spot the current step is trying to reach.
    Vector2D targetPosition{};
    // The direction the player is currently asking for.
    Vector2D inputDirection{};
    // The direction we are actually following right now, so the player finishes
    // the current tile before changing direction.
    Vector2D stepDirection{};
};

struct Sprite {
    SDL_Texture* texture = nullptr;
    SDL_FRect src{};
    SDL_FRect dst{};
    // A render-only offset lets taller character art keep a one-tile gameplay
    // position, so movement and collision can stay simple while the sprite draws upward.
    Vector2D offset{};
};

struct Collider {
    std::string tag;
    SDL_FRect rect{};
};

struct Animation {
    std::unordered_map<std::string, AnimationClip> clips{};
    std::string currentClip{};
    float time{};
    int currentFrame{};
    float speed = 0.1f;
};

struct Camera {
    SDL_FRect view;
    float worldWidth;
    float worldHeight;
};

struct PlayerTag{};
