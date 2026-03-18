#pragma once

#include <memory>
#include <vector>

#include "utils/Vector2D.h"
#include "../Component.h"
#include "../Entity.h"

class MovementSystem {
public:
    void update(std::vector<std::unique_ptr<Entity>>& entities, float dt) {
        for (auto& entity : entities) {
            if (!entity) continue;

            if (entity->hasComponent<Transform>() && entity->hasComponent<Velocity>()) {
                auto& t = entity->getComponent<Transform>();
                auto& v = entity->getComponent<Velocity>();
                // t.position.x += 60.0f * dt;
                // t.position.y += 60.0f * dt;

                t.oldPosition = t.position;

                Vector2D directionVector = v.direction;
                
                // normalizing
                directionVector.normalize();
                
                // Vector2D needs an operation function to multiply a float to itself
                Vector2D velocityVector = directionVector * v.speed;

                // Vector2D velocityVector2 = v.speed * directionVector;
                
                t.position += (velocityVector * dt);
            }       
        }
    }
};
