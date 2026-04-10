#pragma once

#include <memory>
#include <vector>

// We use the collision helper here so we can make sure the next tile is open
// before the player starts moving toward it.
#include "utils/Collision.h"
#include "../Component.h"
#include "../Entity.h"
#include "Map.h"

class MovementSystem {
public:
    void update(std::vector<std::unique_ptr<Entity>>& entities, float dt, const Map& map) {
        for (auto& entity : entities) {
            if (!entity) continue;

            // Only tile-moving entities come through here, and they need a
            // collider because we check the next tile before moving.
            if (entity->hasComponent<Transform>() &&
                entity->hasComponent<GridMovement>() && entity->hasComponent<Collider>()) {
                auto& t = entity->getComponent<Transform>();
                auto& grid = entity->getComponent<GridMovement>();
                auto& collider = entity->getComponent<Collider>();

                t.oldPosition = t.position;

                // If there is no active step, we can decide whether to start one.
                if (grid.stepDirection == Vector2D(0.0f, 0.0f)) {
                    if (grid.inputDirection == Vector2D(0.0f, 0.0f)) continue;

                    // If two keys are held, pick just one direction so the player
                    // moves one tile up, down, left, or right, never diagonally.
                    Vector2D stepInput = grid.inputDirection;
                    if (stepInput.x != 0.0f) stepInput.y = 0.0f;

                    // Turn that direction into the exact spot of the next tile.
                    Vector2D candidatePosition = t.position;
                    candidatePosition += (stepInput * grid.tileSize);

                    // Build the player's collision box at that next tile so we can
                    // see whether the move should be blocked.
                    SDL_FRect candidateRect{candidatePosition.x, candidatePosition.y, collider.rect.w, collider.rect.h};
                    bool blocked = false;
                    const float worldWidth = static_cast<float>(map.width) * grid.tileSize;
                    const float worldHeight = static_cast<float>(map.height) * grid.tileSize;

                    // tiny map forgets some edge wall objects, the player
                    // still cannot step outside the authored map bounds.
                    if (worldWidth > 0.0f && worldHeight > 0.0f) {
                        if (candidatePosition.x < 0.0f ||
                            candidatePosition.y < 0.0f ||
                            (candidatePosition.x + collider.rect.w) > worldWidth ||
                            (candidatePosition.y + collider.rect.h) > worldHeight) {
                            blocked = true;
                        }
                    }

                    // If that next tile overlaps a wall, do not start the step.
                    if (!blocked) {
                        for (const auto& other : entities) {
                            if (!other || other.get() == entity.get() || !other->hasComponent<Collider>()) continue;
                            const auto& otherCollider = other->getComponent<Collider>();
                            if (otherCollider.tag == "wall" && Collision::AABB(candidateRect, otherCollider.rect)) {
                                blocked = true;
                                break;
                            }
                        }
                    }

                    if (blocked) continue;

                    // Save where this step is going and which way it is moving so
                    // the player finishes this tile cleanly.
                    grid.targetPosition = candidatePosition;
                    grid.stepDirection = stepInput;
                }

                // Move in the chosen direction at the set speed until we reach
                // the tile we picked earlier.
                Vector2D movement = grid.stepDirection;
                movement.normalize();
                t.position += (movement * grid.speed * dt);

                // If this frame reaches or goes past the target, place the
                // player exactly on that tile and mark the step as finished.
                Vector2D remaining = grid.targetPosition - t.position;
                if ((remaining.x * grid.stepDirection.x) + (remaining.y * grid.stepDirection.y) <= 0.0f) {
                    t.position = grid.targetPosition;
                    grid.stepDirection = Vector2D(0.0f, 0.0f);
                }
            }
        }
    }
};
