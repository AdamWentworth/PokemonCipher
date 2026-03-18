#pragma once

#include <memory>
#include <vector>

#include "utils/Vector2D.h"
#include "../Component.h"
#include "../Entity.h"
#include "../../TextureManager.h"

class RenderSystem {
public:
    void render(std::vector<std::unique_ptr<Entity>>& entities) {

        Entity* cameraEntity = nullptr;

        for (auto& e : entities) {
            if (e->hasComponent<Camera>()) {
                cameraEntity = e.get();
                break;
            }   
        }

        if (!cameraEntity) return;
        auto& cam =  cameraEntity->getComponent<Camera>();

        for (auto& entity : entities) {
            if (!entity) continue;

            if (entity->hasComponent<Transform>() && entity->hasComponent<Sprite>()) {
                auto& t = entity->getComponent<Transform>();
                auto& sprite = entity->getComponent<Sprite>();

                sprite.dst.x = t.position.x - cam.view.x;
                sprite.dst.y = t.position.y - cam.view.y;

                if (entity->hasComponent<Animation>()) {
                    auto& anim = entity->getComponent<Animation>();
                    auto clipIt = anim.clips.find(anim.currentClip);
                    if (clipIt != anim.clips.end() && !clipIt->second.frameIndices.empty()) {
                        int frameIndex = anim.currentFrame;
                        if (frameIndex < 0 || static_cast<std::size_t>(frameIndex) >= clipIt->second.frameIndices.size()) {
                            frameIndex = 0;
                        }
                        sprite.src = clipIt->second.frameIndices[frameIndex];
                    }
                }

                TextureManager::draw(sprite.texture, sprite.src, sprite.dst);
            }
        }
    }
};
