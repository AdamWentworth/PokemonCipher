#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include <SDL3/SDL_keyboard.h>

#include "engine/ecs/Component.h"
#include "engine/ecs/Entity.h"
#include "engine/utils/Collision.h"

class GridMovementSystem {
public:
    void setInputEnabled(const bool enabled) {
        inputEnabled_ = enabled;
    }

    void setWorldBounds(const float worldWidth, const float worldHeight) {
        worldWidth_ = worldWidth;
        worldHeight_ = worldHeight;
    }

    void update(std::vector<std::unique_ptr<Entity>>& entities, const float dt) const {
        constexpr float kBlockedBumpDuration = 0.12f;
        constexpr float kBlockedBumpAnimationMultiplier = 2.0f;

        const bool* keyboardState = nullptr;
        if (inputEnabled_) {
            keyboardState = SDL_GetKeyboardState(nullptr);
        }

        if (inputEnabled_ && !keyboardState) {
            return;
        }

        for (auto& entity : entities) {
            if (!entity ||
                !entity->hasComponent<PlayerTag>() ||
                !entity->hasComponent<Transform>() ||
                !entity->hasComponent<Velocity>() ||
                !entity->hasComponent<GridMovement>() ||
                !entity->hasComponent<Collider>()) {
                continue;
            }

            auto& transform = entity->getComponent<Transform>();
            auto& velocity = entity->getComponent<Velocity>();
            auto& gridMovement = entity->getComponent<GridMovement>();
            auto& collider = entity->getComponent<Collider>();
            Animation* animation = entity->hasComponent<Animation>()
                ? &entity->getComponent<Animation>()
                : nullptr;

            auto setAnimationBumpState = [animation, kBlockedBumpAnimationMultiplier](const bool isBumping) {
                if (!animation) {
                    return;
                }

                animation->speed = animation->baseSpeed * (isBumping ? kBlockedBumpAnimationMultiplier : 1.0f);
            };

            transform.oldPosition = transform.position;

            if (gridMovement.isMoving) {
                setAnimationBumpState(false);
                updateCurrentStep(transform, velocity, gridMovement, dt);
                continue;
            }

            const Vector2D requestedDirection = inputEnabled_
                ? readRequestedDirection(keyboardState)
                : Vector2D(0.0f, 0.0f);
            if (requestedDirection == Vector2D(0.0f, 0.0f)) {
                if (gridMovement.bumpTimeRemaining > 0.0f) {
                    gridMovement.bumpTimeRemaining = std::max(0.0f, gridMovement.bumpTimeRemaining - dt);
                    velocity.direction = gridMovement.bumpDirection;
                    setAnimationBumpState(true);
                    if (gridMovement.bumpTimeRemaining <= 0.0f) {
                        velocity.direction = Vector2D(0.0f, 0.0f);
                        gridMovement.bumpDirection = Vector2D(0.0f, 0.0f);
                        setAnimationBumpState(false);
                    }
                } else {
                    velocity.direction = Vector2D(0.0f, 0.0f);
                    setAnimationBumpState(false);
                }
                continue;
            }

            const Vector2D candidatePosition = transform.position + (requestedDirection * gridMovement.tileSize);
            if (isBlocked(
                candidatePosition,
                collider.rect.w,
                collider.rect.h,
                entities,
                entity.get(),
                worldWidth_,
                worldHeight_)) {
                // FireRed behavior: turn toward blocked direction and play walk-in-place bump anim.
                velocity.direction = requestedDirection;
                gridMovement.bumpDirection = requestedDirection;
                gridMovement.bumpTimeRemaining = kBlockedBumpDuration;
                setAnimationBumpState(true);
                continue;
            }

            gridMovement.targetPosition = candidatePosition;
            gridMovement.stepDirection = requestedDirection;
            gridMovement.bumpTimeRemaining = 0.0f;
            gridMovement.bumpDirection = Vector2D(0.0f, 0.0f);
            gridMovement.isMoving = true;
            velocity.direction = requestedDirection;
            gridMovement.walkStartFrame = gridMovement.walkAlternation ? 2 : 0;
            gridMovement.walkAlternation = !gridMovement.walkAlternation;
            gridMovement.applyWalkStartFrame = true;
            setAnimationBumpState(false);

            updateCurrentStep(transform, velocity, gridMovement, dt);
        }
    }

private:
    bool inputEnabled_ = true;
    float worldWidth_ = 0.0f;
    float worldHeight_ = 0.0f;

    static Vector2D readRequestedDirection(const bool* keyboardState) {
        Vector2D direction{};

        if (keyboardState[SDL_SCANCODE_W] || keyboardState[SDL_SCANCODE_UP]) {
            direction.y = -1.0f;
        } else if (keyboardState[SDL_SCANCODE_S] || keyboardState[SDL_SCANCODE_DOWN]) {
            direction.y = 1.0f;
        } else if (keyboardState[SDL_SCANCODE_A] || keyboardState[SDL_SCANCODE_LEFT]) {
            direction.x = -1.0f;
        } else if (keyboardState[SDL_SCANCODE_D] || keyboardState[SDL_SCANCODE_RIGHT]) {
            direction.x = 1.0f;
        }

        return direction;
    }

    static void updateCurrentStep(
        Transform& transform,
        Velocity& velocity,
        GridMovement& gridMovement,
        const float dt
    ) {
        Vector2D toTarget = gridMovement.targetPosition - transform.position;
        const float distanceToTarget = std::sqrt((toTarget.x * toTarget.x) + (toTarget.y * toTarget.y));
        const float stepDistance = velocity.speed * dt;

        if (distanceToTarget <= stepDistance || distanceToTarget < 0.001f) {
            transform.position = gridMovement.targetPosition;
            gridMovement.isMoving = false;
            gridMovement.stepDirection = Vector2D(0.0f, 0.0f);
            velocity.direction = Vector2D(0.0f, 0.0f);
            return;
        }

        transform.position += gridMovement.stepDirection * stepDistance;
        velocity.direction = gridMovement.stepDirection;
    }

    static bool isBlocked(
        const Vector2D& candidatePosition,
        const float width,
        const float height,
        const std::vector<std::unique_ptr<Entity>>& entities,
        const Entity* movingEntity,
        const float worldWidth,
        const float worldHeight
    ) {
        if (worldWidth > 0.0f && worldHeight > 0.0f) {
            if (candidatePosition.x < 0.0f ||
                candidatePosition.y < 0.0f ||
                (candidatePosition.x + width) > worldWidth ||
                (candidatePosition.y + height) > worldHeight) {
                return true;
            }
        }

        SDL_FRect candidateRect{};
        candidateRect.x = candidatePosition.x;
        candidateRect.y = candidatePosition.y;
        candidateRect.w = width;
        candidateRect.h = height;

        for (const auto& entity : entities) {
            if (!entity || entity.get() == movingEntity || !entity->hasComponent<Collider>()) {
                continue;
            }

            const auto& collider = entity->getComponent<Collider>();
            if (collider.tag != "wall" && collider.tag != "npc") {
                continue;
            }

            if (Collision::AABB(candidateRect, collider.rect)) {
                return true;
            }
        }

        return false;
    }
};
