#pragma once

#include <memory>
#include <vector>

#include "ecs/Entity.h"

// forward declaration
class World;

class CollisionSystem {
    public:
        void update(World& world);
    private:
        std::vector<Entity*> queryCollidables(const std::vector<std::unique_ptr<Entity>>& entities);

};