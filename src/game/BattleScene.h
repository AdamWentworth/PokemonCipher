#pragma once

#include <functional>
#include <string>

#include "engine/TextureManager.h"
#include "engine/manager/Scene.h"
#include "game/encounters/WildEncounterService.h"
#include "game/state/GameState.h"

class BattleScene : public Scene {
public:
    BattleScene(
        TextureManager& textureManager,
        GameState& gameState,
        WildEncounter encounter,
        std::function<void()> onBattleComplete
    );

    void handleEvent(const SDL_Event& event) override;
    void update(float dt) override;
    void render() override;

private:
    enum class Action {
        Attack = 0,
        Dodge = 1,
        Block = 2,
        Run = 3
    };

    enum class Phase {
        CommandSelect,
        Result
    };

    void moveSelection(int direction);
    void confirmSelection();
    const char* actionLabel(int index) const;

    static constexpr int kActionCount = 4;

    TextureManager& textureManager_;
    GameState& gameState_;
    WildEncounter encounter_{};
    std::function<void()> onBattleComplete_;
    Phase phase_ = Phase::CommandSelect;
    int selectedAction_ = 0;
    std::string resultText_;
};
