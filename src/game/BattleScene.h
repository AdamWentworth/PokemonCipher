#pragma once

#include <deque>
#include <functional>
#include <random>
#include <string>

#include "engine/TextureManager.h"
#include "engine/manager/Scene.h"
#include "game/data/SpeciesRegistry.h"
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
    struct Combatant {
        int speciesId = 0;
        std::string name = "POKEMON";
        int level = 1;
        int maxHp = 1;
        int hp = 1;
        int attack = 1;
        int defense = 1;
        std::string spritePath;
    };

    enum class Action {
        Attack = 0,
        Dodge = 1,
        Block = 2,
        Run = 3
    };

    enum class Phase {
        IntroMessage,
        CommandSelect,
        ResolveMessages,
        Complete
    };

    void initializeCombatants();
    Combatant makePlayerCombatant() const;
    Combatant makeWildCombatant() const;

    void moveSelection(int direction);
    void confirmSelection();
    void resolvePlayerAction(Action action);
    void enemyTurn();
    void enqueueMessage(std::string text);
    void advanceMessage();
    int computeDamage(const Combatant& attacker, const Combatant& defender, int basePower);
    bool rollPercent(int chancePercent);

    void renderBackground(SDL_Renderer* renderer) const;
    void renderCombatants(SDL_Renderer* renderer) const;
    void renderStatusPanels(SDL_Renderer* renderer) const;
    void renderMessageWindow(SDL_Renderer* renderer) const;
    void renderCommandWindow(SDL_Renderer* renderer) const;
    static void drawFrame(SDL_Renderer* renderer, const SDL_FRect& rect, SDL_Color fill, SDL_Color border);
    static void drawHpBar(SDL_Renderer* renderer, const SDL_FRect& rect, float ratio);
    void renderLabel(SDL_Renderer* renderer, float x, float y, const std::string& text, SDL_Color color) const;

    const char* actionLabel(int index) const;

    static constexpr int kActionCount = 4;

    TextureManager& textureManager_;
    GameState& gameState_;
    WildEncounter encounter_{};
    std::function<void()> onBattleComplete_;
    Phase phase_ = Phase::IntroMessage;
    int selectedAction_ = 0;
    Combatant player_{};
    Combatant wild_{};
    bool playerDodging_ = false;
    bool playerBlocking_ = false;
    bool battleEnded_ = false;
    std::deque<std::string> messageQueue_{};
    std::mt19937 rng_{};
};
