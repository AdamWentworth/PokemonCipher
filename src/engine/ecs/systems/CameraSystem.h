#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include "../Component.h"
#include "../Entity.h"

class CameraSystem {
public:
    void update(const std::vector<std::unique_ptr<Entity>>& entities) const {
        Entity* playerEntity = nullptr;

        for (const auto& entity : entities) {
            if (entity && entity->hasComponent<PlayerTag>() && entity->hasComponent<Transform>()) {
                playerEntity = entity.get();
                break;
            }
        }

        if (!playerEntity) {
            return;
        }

        const auto& playerTransform = playerEntity->getComponent<Transform>();

        for (const auto& entity : entities) {
            if (!entity || !entity->hasComponent<Camera>()) {
                continue;
            }

            auto& camera = entity->getComponent<Camera>();

            camera.view.x = playerTransform.position.x - (camera.view.w * 0.5f);
            camera.view.y = playerTransform.position.y - (camera.view.h * 0.5f);

            if (camera.view.x < 0.0f) {
                camera.view.x = 0.0f;
            }
            if (camera.view.y < 0.0f) {
                camera.view.y = 0.0f;
            }

            const float maxCameraX = std::max(0.0f, camera.worldWidth - camera.view.w);
            const float maxCameraY = std::max(0.0f, camera.worldHeight - camera.view.h);

            if (camera.view.x > maxCameraX) {
                camera.view.x = maxCameraX;
            }
            if (camera.view.y > maxCameraY) {
                camera.view.y = maxCameraY;
            }

            // Keep camera on whole pixels for crisp tile rendering.
            camera.view.x = std::round(camera.view.x);
            camera.view.y = std::round(camera.view.y);
        }
    }
};
