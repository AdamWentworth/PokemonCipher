#pragma once

#include <SDL3/SDL_events.h>
#include <string>
#include "../ecs/World.h"

class Scene {
public:
    Scene(const char* sceneName, const char* mapPath, int windowWidth, int windowHeight);

    void update(float dt, SDL_Event& e) {
        world.update(dt, e);
    }

    void render() {
        world.render();
    }

    World world;

    const std::string& getName() const {
        return name;
    }

private:
    std::string name;
};
