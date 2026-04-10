#pragma once

#include <memory>
#include <vector>

#include "../Component.h"
#include "../Entity.h"

class CameraSystem {
    public:
    void update(const std::vector<std::unique_ptr<Entity>>& entities) {

        Entity* playerEntity = nullptr;

        for (auto& e: entities) {
            if (e->hasComponent<PlayerTag>()) {
                playerEntity = e.get();
                break;
            }
        }
        if (!playerEntity) return;

        auto& playerTransform = playerEntity->getComponent<Transform>();

        for (auto& e:entities) {
            if (e->hasComponent<Camera>()) {

                auto& cam = e->getComponent<Camera>();
                // A small room can be smaller than the camera view, so clamp
                // the camera limit to zero instead of letting it go negative.
                float maxX = cam.worldWidth - cam.view.w;
                float maxY = cam.worldHeight - cam.view.h;
                if (maxX < 0.0f) {
                    maxX = 0.0f;
                }
                if (maxY < 0.0f) {
                    maxY = 0.0f;
                }

                cam.view.x = playerTransform.position.x - cam.view.w / 2;
                cam.view.y = playerTransform.position.y - cam.view.h / 2;

                if (cam.view.x < 0 ) {
                    cam.view.x = 0;
                }

                if (cam.view.y < 0) {
                    cam.view.y = 0;
                }

                if (cam.view.x > maxX)
                    cam.view.x = maxX;

                if (cam.view.y > maxY)
                    cam.view.y = maxY;
            }
        }
    }
};
