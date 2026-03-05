#pragma once

#include <functional>

#include <SDL3/SDL_keycode.h>

#include "engine/TextureManager.h"
#include "engine/manager/Scene.h"

enum class TitleMenuSelection {
    NewGame,
    ContinueGame,
    DevJump
};

class TitleScene : public Scene {
public:
    TitleScene(
        TextureManager& textureManager,
        bool hasContinueSlot,
        std::function<void(TitleMenuSelection)> onSelection
    );

    void handleEvent(const SDL_Event& event) override;
    void update(float dt) override;
    void render() override;

private:
    static constexpr int kEntryCount = 3;

    void moveSelection(int direction);
    void confirmSelection();
    static const char* entryLabel(int index, bool hasContinueSlot);

    TextureManager& textureManager_;
    bool hasContinueSlot_ = false;
    int selectedIndex_ = 0;
    std::function<void(TitleMenuSelection)> onSelection_;
};
