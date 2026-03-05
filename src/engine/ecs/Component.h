#pragma once

#include <SDL3/SDL_render.h>

#include <string>
#include <unordered_map>

#include "systems/AnimationClip.h"
#include "engine/utils/Vector2D.h"

struct Transform {
    Vector2D position;
    float rotation = 0.0f;
    float scale = 1.0f;
    Vector2D oldPosition;
};

struct Velocity {
    Vector2D direction;
    float speed = 0.0f;
};

struct GridMovement {
    float tileSize = 32.0f;
    bool isMoving = false;
    Vector2D targetPosition;
    Vector2D stepDirection;
    float bumpTimeRemaining = 0.0f;
    Vector2D bumpDirection;
    bool walkAlternation = false;
    int walkStartFrame = 0;
    bool applyWalkStartFrame = false;
};

struct Sprite {
    SDL_Texture* texture = nullptr;
    SDL_FRect src{};
    SDL_FRect dst{};
    Vector2D offset{};
    bool mirrorOnRightClip = false;
};

struct Collider {
    std::string tag;
    SDL_FRect rect{};
};

struct Animation {
    std::unordered_map<std::string, AnimationClip> clips;
    std::string currentClip;
    float time = 0.0f;
    int currentFrame = 0;
    float baseSpeed = 0.12f;
    float speed = 0.12f;
};

struct Camera {
    SDL_FRect view{};
    float worldWidth = 0.0f;
    float worldHeight = 0.0f;
};

struct NpcInteraction {
    std::string id;
    std::string scriptId;
    std::string speaker;
    std::string dialogue;
};

struct PlayerTag {};
struct NpcTag {};
