#include "CollisionSystem.h"
#include "ecs/World.h"
#include "utils/Collision.h"
#include <iostream>

// has two important functions, but they are related
// first is positions the collider with the transform
// next is checking for collisions

void CollisionSystem::update(World &world) {
    const std::vector<Entity*> collidables = queryCollidables(world.getEntities());

    // Update ALL collider rect positions first
    for (auto* e : collidables) {
        auto& t = e->getComponent<Transform>();
        auto& c = e->getComponent<Collider>();
        c.rect.x = t.position.x;
        c.rect.y = t.position.y;
    }

    // Then check collisions
    for (size_t i = 0; i < collidables.size(); i++) {
        auto entityA = collidables[i];
        auto& colliderA = entityA->getComponent<Collider>();

        for (size_t j = i + 1; j < collidables.size(); j++) {
            auto entityB = collidables[j];
            auto& colliderB = entityB->getComponent<Collider>();

            if (Collision::AABB(colliderA, colliderB)) {
                // std::cout << colliderA.tag << " hit: " << colliderB.tag << std::endl; // unchanged
                world.getEventManager().emit(CollisionEvent{entityA, entityB});
            }
        }
    }
}

std::vector<Entity*> CollisionSystem::queryCollidables(const std::vector<std::unique_ptr<Entity>>& entities) {
    std::vector<Entity*> collidables;
    for (auto& e: entities) {
        if (e->hasComponent<Transform>() && e->hasComponent<Collider>()) {
            collidables.push_back(e.get());
        }
    }
    return collidables;
}
