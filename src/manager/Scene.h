#pragma once

#include <SDL3/SDL_events.h>
#include <string>
#include "../ecs/World.h"
#include "../ecs/systems/DialogueSystem.h"
#include "../ecs/systems/StartMenuSystem.h"
#include "../ecs/systems/WaypointSystem.h"

class Scene {
    public:
    Scene(const char* sceneName, const char* mapPath, const char* tilesetPath, int windowWidth, int windowHeight, const SceneSpawnRequest& spawnRequest = {});

    void update(const float dt, SDL_Event &e);
    void render();

    // SceneManager reads this after update so a touched warp tile can ask for
    // the next map without Scene knowing how scenes are stored.
    bool takePendingSceneChange(SceneChangeRequest& request);

    World world;

    const std::string& getName() const {return name; }
    
    private:
    std::string name; 
    SceneChangeRequest pendingSceneChange{};
    // Dialogue stays here so interaction ids can open a text box without
    // teaching the lower-level map or world code about UI.
    DialogueSystem dialogueSystem;
    // The start menu also lives at the scene layer because it is an overworld
    // overlay, not a low-level map or entity behavior.
    StartMenuSystem startMenuSystem;
    // The waypoint helper owns the simple spawn and warp rules so Scene can
    // stay focused on assembling the map and entities.
    WaypointSystem waypointSystem;
};
