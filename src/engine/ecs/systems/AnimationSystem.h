#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "../Component.h"
#include "../Entity.h"

class AnimationSystem {
public:
    void update(const std::vector<std::unique_ptr<Entity>>& entities, const float dt) const {
        for (const auto& entity : entities) {
            if (!entity || !entity->hasComponent<Animation>() || !entity->hasComponent<Velocity>()) {
                continue;
            }

            auto& animation = entity->getComponent<Animation>();
            const auto& velocity = entity->getComponent<Velocity>();

            auto hasClip = [&animation](const std::string& clipName) {
                const auto it = animation.clips.find(clipName);
                return it != animation.clips.end() && !it->second.frameIndices.empty();
            };

            std::string nextClip = animation.currentClip;
            constexpr float kDirectionEpsilon = 0.01f;
            const float absX = std::abs(velocity.direction.x);
            const float absY = std::abs(velocity.direction.y);

            if (absX > kDirectionEpsilon || absY > kDirectionEpsilon) {
                if (absX >= absY) {
                    nextClip = velocity.direction.x > 0.0f ? "walk_right" : "walk_left";
                } else {
                    nextClip = velocity.direction.y > 0.0f ? "walk_down" : "walk_up";
                }
            } else if (animation.currentClip == "walk_right") {
                nextClip = "idle_right";
            } else if (animation.currentClip == "walk_left") {
                nextClip = "idle_left";
            } else if (animation.currentClip == "walk_down") {
                nextClip = "idle_down";
            } else if (animation.currentClip == "walk_up") {
                nextClip = "idle_up";
            } else if (animation.currentClip.empty()) {
                nextClip = "idle_down";
            }

            if (!hasClip(nextClip)) {
                if (hasClip(animation.currentClip)) {
                    nextClip = animation.currentClip;
                } else if (!animation.clips.empty()) {
                    nextClip = animation.clips.begin()->first;
                } else {
                    continue;
                }
            }

            if (nextClip != animation.currentClip) {
                animation.currentClip = nextClip;
                animation.time = 0.0f;
                animation.currentFrame = 0;
            }

            const auto clipIt = animation.clips.find(animation.currentClip);
            if (clipIt == animation.clips.end() || clipIt->second.frameIndices.empty()) {
                continue;
            }

            if (entity->hasComponent<GridMovement>()) {
                auto& gridMovement = entity->getComponent<GridMovement>();
                const bool isWalkClip = animation.currentClip.rfind("walk_", 0) == 0;
                if (isWalkClip && gridMovement.applyWalkStartFrame) {
                    const int maxFrame = static_cast<int>(clipIt->second.frameIndices.size()) - 1;
                    animation.currentFrame = std::clamp(gridMovement.walkStartFrame, 0, std::max(0, maxFrame));
                    animation.time = 0.0f;
                    gridMovement.applyWalkStartFrame = false;
                } else if (!isWalkClip) {
                    gridMovement.applyWalkStartFrame = false;
                }
            }

            animation.time += dt;
            if (animation.time >= animation.speed) {
                animation.time -= animation.speed;
                const std::size_t totalFrames = clipIt->second.frameIndices.size();
                animation.currentFrame = (animation.currentFrame + 1) % static_cast<int>(totalFrames);
            }
        }
    }
};
