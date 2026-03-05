#pragma once

#include <cmath>
#include <string>

#include <memory>
#include <vector>

#include "../Component.h"
#include "../Entity.h"
#include "engine/TextureManager.h"

class RenderSystem {
public:
    void render(std::vector<std::unique_ptr<Entity>>& entities, TextureManager& textureManager) const {
        Entity* cameraEntity = nullptr;

        for (const auto& entity : entities) {
            if (entity && entity->hasComponent<Camera>()) {
                cameraEntity = entity.get();
                break;
            }
        }

        if (!cameraEntity) {
            return;
        }

        const auto& camera = cameraEntity->getComponent<Camera>();

        for (const auto& entity : entities) {
            if (!entity || !entity->hasComponent<Transform>() || !entity->hasComponent<Sprite>()) {
                continue;
            }

            auto& transform = entity->getComponent<Transform>();
            auto& sprite = entity->getComponent<Sprite>();

            sprite.dst.x = std::round(transform.position.x - camera.view.x + sprite.offset.x);
            sprite.dst.y = std::round(transform.position.y - camera.view.y + sprite.offset.y);

            if (entity->hasComponent<Animation>()) {
                auto& animation = entity->getComponent<Animation>();
                const auto clipIt = animation.clips.find(animation.currentClip);
                if (clipIt != animation.clips.end() && !clipIt->second.frameIndices.empty()) {
                    int frameIndex = animation.currentFrame;
                    if (frameIndex < 0 || static_cast<std::size_t>(frameIndex) >= clipIt->second.frameIndices.size()) {
                        frameIndex = 0;
                    }
                    sprite.src = clipIt->second.frameIndices[frameIndex];
                }
            }

            SDL_FlipMode flip = SDL_FLIP_NONE;
            if (sprite.mirrorOnRightClip && entity->hasComponent<Animation>()) {
                const auto& animation = entity->getComponent<Animation>();
                const std::string& clip = animation.currentClip;
                if (clip == "walk_right" || clip == "idle_right") {
                    flip = SDL_FLIP_HORIZONTAL;
                }
            }

            textureManager.draw(sprite.texture, sprite.src, sprite.dst, flip);
        }
    }
};
