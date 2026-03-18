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
    int windowWidth;
    int windowHeight;
};

class SceneManager {
    std::unordered_map<std::string, SceneParams> sceneParams;
    std::string pendingScene;

    void changeScene(const std::string& name) {
        auto it = sceneParams.find(name);
        if (it != sceneParams.end()) {
            const auto& params = it->second;
            currentScene = std::make_unique<Scene>(
                params.name,
                params.mapPath,
                params.windowWidth,
                params.windowHeight
            );
        } else {
            std::cerr << "Scene " << name << " not found!" << std::endl;
        }
    }

public:
    std::unique_ptr<Scene> currentScene;

    void load(const char* sceneName, const char* mapPath, int windowWidth, int windowHeight) {
        sceneParams[sceneName] = {sceneName, mapPath, windowWidth, windowHeight};
    }

    void loadScene(const char* sceneName, const char* mapPath, int windowWidth, int windowHeight) {
        load(sceneName, mapPath, windowWidth, windowHeight);
    }

    void changeSceneDeferred(const std::string& name) {
        pendingScene = name;
    }

    void update(const float dt, SDL_Event& e) {
        if (currentScene) {
            currentScene->update(dt, e);
        }

        if (!pendingScene.empty()) {
            changeScene(pendingScene);
            pendingScene.clear();
        }
    }

    void render() const {
        if (currentScene) {
            currentScene->render();
        }
    }
};
