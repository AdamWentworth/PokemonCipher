#pragma once

#include <memory>
#include <vector>

#include "../Component.h"
#include "../Entity.h"

class DestructionSystem {
public:
    void update(const std::vector<std::unique_ptr<Entity>>& entities) {

        Entity* cameraEntity = nullptr;

        for (auto& e: entities) {
            if (e->hasComponent<Camera>()) {
                cameraEntity = e.get();
                break;
            }
        }

        if (!cameraEntity) return;
        auto& cam = cameraEntity->getComponent<Camera>();

        for (auto& e: entities) {
            if (e->hasComponent<Transform>() && e->hasComponent<ProjectileTag>()){
                auto& t = e->getComponent<Transform>();
                if (
                    t.position.x > cam.view.x + cam.view.w ||
                    t.position.x < cam.view.x ||
                    t.position.y > cam.view.y + cam.view.h ||
                    t.position.y < cam.view.y) {
                    
                    
                    e->destroy();
                }
            }
        }
    }    
};
