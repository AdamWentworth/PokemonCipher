#pragma once

#include <SDL3/SDL_events.h>

class Scene {
public:
    virtual ~Scene() = default;

    virtual void handleEvent(const SDL_Event& event) = 0;
    virtual void update(float dt) = 0;
    virtual void render() = 0;
};
