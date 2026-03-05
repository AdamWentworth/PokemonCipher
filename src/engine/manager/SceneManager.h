#pragma once

#include <SDL3/SDL_events.h>

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "Scene.h"

class SceneManager {
public:
    using SceneFactory = std::function<std::unique_ptr<Scene>()>;

    void registerScene(const std::string& name, SceneFactory factory) {
        sceneFactories_[name] = std::move(factory);
    }

    bool changeScene(const std::string& name) {
        const auto it = sceneFactories_.find(name);
        if (it == sceneFactories_.end()) {
            std::cerr << "Scene '" << name << "' is not registered.\n";
            return false;
        }

        currentScene_ = it->second();
        return static_cast<bool>(currentScene_);
    }

    void handleEvent(const SDL_Event& event) {
        if (currentScene_) {
            currentScene_->handleEvent(event);
        }
    }

    void update(const float dt) {
        if (currentScene_) {
            currentScene_->update(dt);
        }
    }

    void render() const {
        if (currentScene_) {
            currentScene_->render();
        }
    }

    Scene* currentScene() const { return currentScene_.get(); }

private:
    std::unordered_map<std::string, SceneFactory> sceneFactories_;
    std::unique_ptr<Scene> currentScene_;
};
