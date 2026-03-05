#pragma once

#include <algorithm>
#include <memory>
#include <vector>

#include "engine/TextureManager.h"
#include "Entity.h"
#include "systems/AnimationSystem.h"
#include "systems/CameraSystem.h"
#include "systems/CollisionSystem.h"
#include "systems/RenderSystem.h"

class World {
public:
    void update(const float dt) {
        collisions_.clear();
        collisionSystem_.update(entities_, collisions_);

        animationSystem_.update(entities_, dt);

        cleanup();
    }

    void updateCamera() {
        cameraSystem_.update(entities_);
    }

    void render(TextureManager& textureManager) {
        renderSystem_.render(entities_, textureManager);
    }

    Entity& createEntity() {
        entities_.emplace_back(std::make_unique<Entity>());
        return *entities_.back();
    }

    std::vector<std::unique_ptr<Entity>>& entities() {
        return entities_;
    }

    const std::vector<std::unique_ptr<Entity>>& entities() const {
        return entities_;
    }

    const std::vector<CollisionPair>& collisions() const {
        return collisions_;
    }

    void clear() {
        collisions_.clear();
        entities_.clear();
    }

    template <typename T>
    Entity* findFirstWith() {
        for (const auto& entity : entities_) {
            if (entity && entity->hasComponent<T>()) {
                return entity.get();
            }
        }
        return nullptr;
    }

    template <typename T>
    const Entity* findFirstWith() const {
        for (const auto& entity : entities_) {
            if (entity && entity->hasComponent<T>()) {
                return entity.get();
            }
        }
        return nullptr;
    }

private:
    void cleanup() {
        std::erase_if(entities_, [](const std::unique_ptr<Entity>& entity) {
            return !entity || !entity->isActive();
        });
    }

    std::vector<std::unique_ptr<Entity>> entities_;

    RenderSystem renderSystem_;
    CollisionSystem collisionSystem_;
    AnimationSystem animationSystem_;
    CameraSystem cameraSystem_;

    std::vector<CollisionPair> collisions_;
};
