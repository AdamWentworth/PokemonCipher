#pragma once

#include <SDL3/SDL_events.h>
#include <string>
#include "../ecs/World.h"
#include "../ecs/systems/WaypointSystem.h"

class Scene {
    public:
    Scene(const char* sceneName, const char* mapPath, const char* tilesetPath, int windowWidth, int windowHeight, const SceneSpawnRequest& spawnRequest = {});

    void update(const float dt, SDL_Event &e);

    void render() {
        world.render();
    }

    // SceneManager reads this after update so a touched warp tile can ask for
    // the next map without Scene knowing how scenes are stored.
    bool takePendingSceneChange(SceneChangeRequest& request);

    World world;

    const std::string& getName() const {return name; }
    
    private:
    std::string name; 
    SceneChangeRequest pendingSceneChange{};
    // The waypoint helper owns the simple spawn and warp rules so Scene can
    // stay focused on assembling the map and entities.
    WaypointSystem waypointSystem;
    void createProjectile(Vector2D pos, Vector2D dir, int speed);
};
