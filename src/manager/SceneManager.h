#pragma once

#include <SDL3/SDL_events.h>

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "Scene.h"

struct SceneParams {
    const char* name;
    const char* mapPath;
    const char* tilesetPath;
    int windowWidth;
    int windowHeight;
};

class SceneManager {
    std::unordered_map<std::string, SceneParams> sceneParams;
    SceneChangeRequest pendingSceneChange{};

    void changeScene(const SceneChangeRequest& request) {
        auto it = sceneParams.find(request.sceneName);
        if (it != sceneParams.end()) {
            const auto& params = it->second;
            currentScene = std::make_unique<Scene>(
                params.name,
                params.mapPath,
                params.tilesetPath,
                params.windowWidth,
                params.windowHeight,
                request.spawn
            );
        } else {
            std::cerr << "Scene " << request.sceneName << " not found!" << std::endl;
        }
    }

    public:
    std::unique_ptr<Scene> currentScene;

    void load(const char* sceneName, const char* mapPath, const char* tilesetPath, int windowWidth, int windowHeight) {
        // Each map keeps its own tileset path so scene changes can load the
        // right art without extra special-case code.
        sceneParams[sceneName] = {sceneName, mapPath, tilesetPath, windowWidth, windowHeight};
    }

    void loadScene(const char* sceneName, const char* mapPath, const char* tilesetPath, int windowWidth, int windowHeight) {
        load(sceneName, mapPath, tilesetPath, windowWidth, windowHeight);
    }

    void changeSceneDeferred(const std::string& name, const SceneSpawnRequest& spawn = {}) {
        // The scene name says which map to load next, and the spawn request
        // says where inside that map the player should appear.
        pendingSceneChange.sceneName = name;
        pendingSceneChange.spawn = spawn;
    }

    void update(const float dt, SDL_Event& e) {
        if (currentScene) {
            currentScene->update(dt, e);

            SceneChangeRequest request;
            if (currentScene->takePendingSceneChange(request)) {
                pendingSceneChange = request;
            }
        }

        if (!pendingSceneChange.sceneName.empty()) {
            changeScene(pendingSceneChange);
            pendingSceneChange = {};
        }
    }

    void render() const {
        if (currentScene) {
            currentScene->render();
        }
    }
};
