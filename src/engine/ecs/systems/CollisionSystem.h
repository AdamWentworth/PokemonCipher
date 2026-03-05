#pragma once

#include <memory>
#include <vector>

#include "engine/ecs/Entity.h"

struct CollisionPair {
    Entity* entityA = nullptr;
    Entity* entityB = nullptr;
};

class CollisionSystem {
public:
    void update(const std::vector<std::unique_ptr<Entity>>& entities, std::vector<CollisionPair>& collisions) const;

private:
    std::vector<Entity*> queryCollidables(const std::vector<std::unique_ptr<Entity>>& entities) const;
};
