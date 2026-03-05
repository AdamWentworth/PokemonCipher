#include "engine/ecs/systems/CollisionSystem.h"

#include "engine/utils/Collision.h"

void CollisionSystem::update(const std::vector<std::unique_ptr<Entity>>& entities, std::vector<CollisionPair>& collisions) const {
    const std::vector<Entity*> collidables = queryCollidables(entities);

    for (Entity* entity : collidables) {
        auto& transform = entity->getComponent<Transform>();
        auto& collider = entity->getComponent<Collider>();

        collider.rect.x = transform.position.x;
        collider.rect.y = transform.position.y;
    }

    for (std::size_t i = 0; i < collidables.size(); ++i) {
        Entity* entityA = collidables[i];
        auto& colliderA = entityA->getComponent<Collider>();

        for (std::size_t j = i + 1; j < collidables.size(); ++j) {
            Entity* entityB = collidables[j];
            auto& colliderB = entityB->getComponent<Collider>();

            if (Collision::AABB(colliderA, colliderB)) {
                collisions.push_back(CollisionPair{entityA, entityB});
            }
        }
    }
}

std::vector<Entity*> CollisionSystem::queryCollidables(const std::vector<std::unique_ptr<Entity>>& entities) const {
    std::vector<Entity*> collidables;

    for (const auto& entity : entities) {
        if (entity && entity->hasComponent<Transform>() && entity->hasComponent<Collider>()) {
            collidables.push_back(entity.get());
        }
    }

    return collidables;
}
