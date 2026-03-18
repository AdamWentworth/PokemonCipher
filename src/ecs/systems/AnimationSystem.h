#pragma once

#include "AnimationClip.h"
#include "ecs/Component.h"
#include "ecs/Entity.h"

class AnimationSystem {
    public:
    void update(const std::vector<std::unique_ptr<Entity>>& entities, float dt) {
        for (auto& e : entities) {
            if (e->hasComponent<Animation>() && e->hasComponent<Velocity>()) {
                auto& anim = e->getComponent<Animation>();
                auto& velocity = e->getComponent<Velocity>();
                auto hasClip = [&anim](const std::string& clipName) {
                    auto it = anim.clips.find(clipName);
                    return it != anim.clips.end() && !it->second.frameIndices.empty();
                };

                std::string newClip = anim.currentClip;

                if (velocity.direction.x > 0.0f) {
                    newClip = "walk_right";
                } else if (velocity.direction.x < 0.0f) {
                    newClip = "walk_left";
                } else if (velocity.direction.y > 0.0f) {
                    newClip = "walk_down";
                } else if (velocity.direction.y < 0.0f) {
                    newClip = "walk_up";
                } else if (anim.currentClip == "walk_right") {
                    newClip = "idle_right";
                } else if (anim.currentClip == "walk_left") {
                    newClip = "idle_left";
                } else if (anim.currentClip == "walk_down") {
                    newClip = "idle_down";
                } else if (anim.currentClip == "walk_up") {
                    newClip = "idle_up";
                } else if (anim.currentClip.empty()) {
                    newClip = "idle_down";
                } else {
                    newClip = anim.currentClip;
                }

                if (!hasClip(newClip)) {
                    if (hasClip(anim.currentClip)) {
                        newClip = anim.currentClip;
                    } else if (hasClip("idle_right")) {
                        newClip = "idle_right";
                    } else if (!anim.clips.empty()) {
                        newClip = anim.clips.begin()->first;
                    } else {
                        continue;
                    }
                }

                if (newClip != anim.currentClip) {
                    anim.currentClip = newClip;
                    anim.time = 0.0f;
                    anim.currentFrame = 0;
                }
                auto clipIt = anim.clips.find(anim.currentClip);
                if (clipIt == anim.clips.end() || clipIt->second.frameIndices.empty()) {
                    continue;
                }

                float frameSpeed = anim.speed;
                auto& clip = clipIt->second;
                anim.time += dt;

                if (anim.time >= frameSpeed) {
                    anim.time -= frameSpeed;
                    std::size_t totalAnimationFrames = clip.frameIndices.size();
                    anim.currentFrame=(anim.currentFrame + 1) % totalAnimationFrames;
                }
            }
        }
    }
};
