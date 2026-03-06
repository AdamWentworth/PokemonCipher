#include "game/BattleScene.h"

#include <algorithm>
#include <sstream>
#include <utility>

#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>

namespace {
constexpr int kConfirmKeys[] = {SDLK_RETURN, SDLK_KP_ENTER, SDLK_SPACE, SDLK_Z, SDLK_X};
}

BattleScene::BattleScene(
    TextureManager& textureManager,
    GameState& gameState,
    WildEncounter encounter,
    std::function<void()> onBattleComplete
) : textureManager_(textureManager),
    gameState_(gameState),
    encounter_(std::move(encounter)),
    onBattleComplete_(std::move(onBattleComplete)),
    rng_(static_cast<std::mt19937::result_type>(SDL_GetTicks())) {
    gameState_.setFlag("in_battle", true);
    gameState_.setVar("last_wild_species_id", encounter_.speciesId);
    gameState_.setVar("last_wild_level", encounter_.level);
    initializeCombatants();

    std::ostringstream introLine;
    introLine << "A wild " << wild_.name << " appeared!";
    enqueueMessage(introLine.str());
}

void BattleScene::handleEvent(const SDL_Event& event) {
    if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) {
        return;
    }

    const SDL_Keycode key = event.key.key;
    const bool isConfirm = std::any_of(
        std::begin(kConfirmKeys),
        std::end(kConfirmKeys),
        [key](const int candidate) { return key == candidate; }
    );

    if (isConfirm) {
        if (phase_ == Phase::CommandSelect) {
            confirmSelection();
        } else {
            advanceMessage();
        }
        return;
    }

    if (phase_ != Phase::CommandSelect) {
        return;
    }

    if (key == SDLK_UP || key == SDLK_W) {
        moveSelection(-2);
    } else if (key == SDLK_DOWN || key == SDLK_S) {
        moveSelection(2);
    } else if (key == SDLK_LEFT || key == SDLK_A) {
        moveSelection(-1);
    } else if (key == SDLK_RIGHT || key == SDLK_D) {
        moveSelection(1);
    }
}

void BattleScene::update(const float dt) {
    (void)dt;
}

void BattleScene::render() {
    SDL_Renderer* renderer = textureManager_.renderer();
    if (!renderer) {
        return;
    }

    renderBackground(renderer);
    renderCombatants(renderer);
    renderStatusPanels(renderer);
    renderMessageWindow(renderer);
    renderCommandWindow(renderer);
}

void BattleScene::initializeCombatants() {
    player_ = makePlayerCombatant();
    wild_ = makeWildCombatant();
}

BattleScene::Combatant BattleScene::makePlayerCombatant() const {
    Combatant combatant{};
    const auto& party = gameState_.party();
    const PartyPokemon playerMon = party.empty() ? PartyPokemon{133, 5, true} : party.front();

    combatant.speciesId = playerMon.speciesId;
    combatant.level = std::max(1, playerMon.level);
    combatant.maxHp = std::max(12, 14 + (combatant.level * 3));
    combatant.hp = combatant.maxHp;
    combatant.attack = std::max(5, 7 + (combatant.level * 2));
    combatant.defense = std::max(4, 6 + (combatant.level * 2));

    if (const SpeciesDefinition* species = getSpeciesRegistry().find(combatant.speciesId)) {
        if (!species->name.empty()) {
            combatant.name = species->name;
        }
        combatant.spritePath = species->frontSpritePath;
    }

    return combatant;
}

BattleScene::Combatant BattleScene::makeWildCombatant() const {
    Combatant combatant{};
    combatant.speciesId = encounter_.speciesId;
    combatant.level = std::max(1, encounter_.level);
    combatant.maxHp = std::max(8, 10 + (combatant.level * 2));
    combatant.hp = combatant.maxHp;
    combatant.attack = std::max(3, 5 + (combatant.level * 2));
    combatant.defense = std::max(2, 4 + (combatant.level * 2));

    if (!encounter_.speciesName.empty()) {
        combatant.name = encounter_.speciesName;
    }

    if (const SpeciesDefinition* species = getSpeciesRegistry().find(combatant.speciesId)) {
        if (!species->name.empty()) {
            combatant.name = species->name;
        }
        combatant.spritePath = species->frontSpritePath;
    }

    return combatant;
}

void BattleScene::moveSelection(const int direction) {
    selectedAction_ = (selectedAction_ + kActionCount + direction) % kActionCount;
}

void BattleScene::confirmSelection() {
    if (phase_ != Phase::CommandSelect) {
        return;
    }

    resolvePlayerAction(static_cast<Action>(selectedAction_));
    if (!battleEnded_) {
        enemyTurn();
    }
    phase_ = Phase::ResolveMessages;
}

void BattleScene::resolvePlayerAction(const Action action) {
    playerDodging_ = false;
    playerBlocking_ = false;

    switch (action) {
    case Action::Attack: {
        enqueueMessage(player_.name + " attacks!");
        const int damage = computeDamage(player_, wild_, 8);
        wild_.hp = std::max(0, wild_.hp - damage);

        std::ostringstream damageText;
        damageText << "Wild " << wild_.name << " took " << damage << " damage.";
        enqueueMessage(damageText.str());

        if (wild_.hp <= 0) {
            battleEnded_ = true;
            enqueueMessage("Wild " + wild_.name + " fainted!");
            gameState_.setVar("last_battle_outcome", 1);
        }
        break;
    }
    case Action::Dodge:
        playerDodging_ = true;
        enqueueMessage(player_.name + " readies to dodge.");
        break;
    case Action::Block:
        playerBlocking_ = true;
        enqueueMessage(player_.name + " takes a defensive stance.");
        break;
    case Action::Run:
        if (rollPercent(75)) {
            battleEnded_ = true;
            enqueueMessage("Got away safely!");
            gameState_.setVar("last_battle_outcome", 2);
        } else {
            enqueueMessage("Couldn't escape!");
        }
        break;
    }
}

void BattleScene::enemyTurn() {
    if (battleEnded_) {
        return;
    }

    enqueueMessage("Wild " + wild_.name + " attacks!");

    if (playerDodging_ && rollPercent(70)) {
        enqueueMessage(player_.name + " dodged the attack!");
        return;
    }

    if (playerDodging_) {
        enqueueMessage("Dodge failed!");
    }

    int damage = computeDamage(wild_, player_, 7);
    if (playerBlocking_) {
        damage = std::max(1, damage / 2);
        enqueueMessage(player_.name + " blocked part of the hit.");
    }

    player_.hp = std::max(0, player_.hp - damage);

    std::ostringstream damageText;
    damageText << player_.name << " took " << damage << " damage.";
    enqueueMessage(damageText.str());

    if (player_.hp <= 0) {
        battleEnded_ = true;
        enqueueMessage(player_.name + " fainted...");
        gameState_.setVar("last_battle_outcome", 0);
    }
}

void BattleScene::enqueueMessage(std::string text) {
    if (!text.empty()) {
        messageQueue_.push_back(std::move(text));
    }
}

void BattleScene::advanceMessage() {
    if (!messageQueue_.empty()) {
        messageQueue_.pop_front();
    }

    if (!messageQueue_.empty()) {
        return;
    }

    if (phase_ == Phase::IntroMessage) {
        phase_ = Phase::CommandSelect;
        return;
    }

    if (phase_ == Phase::ResolveMessages && !battleEnded_) {
        phase_ = Phase::CommandSelect;
        return;
    }

    if (battleEnded_ && onBattleComplete_) {
        phase_ = Phase::Complete;
        gameState_.setFlag("in_battle", false);
        auto onComplete = std::move(onBattleComplete_);
        onBattleComplete_ = nullptr;
        onComplete();
    }
}

int BattleScene::computeDamage(const Combatant& attacker, const Combatant& defender, const int basePower) {
    std::uniform_int_distribution<int> variance(-2, 2);
    const int raw = attacker.attack + basePower - (defender.defense / 2) + variance(rng_);
    return std::clamp(raw, 1, 99);
}

bool BattleScene::rollPercent(const int chancePercent) {
    std::uniform_int_distribution<int> roll(1, 100);
    return roll(rng_) <= std::clamp(chancePercent, 0, 100);
}

void BattleScene::renderBackground(SDL_Renderer* renderer) const {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, 98, 188, 245, 255);
    SDL_RenderClear(renderer);

    const SDL_FRect horizon{0.0f, 0.0f, 240.0f, 88.0f};
    drawFrame(renderer, horizon, SDL_Color{208, 240, 255, 255}, SDL_Color{138, 190, 228, 255});

    const SDL_FRect ground{0.0f, 88.0f, 240.0f, 72.0f};
    drawFrame(renderer, ground, SDL_Color{201, 229, 183, 255}, SDL_Color{151, 192, 138, 255});
}

void BattleScene::renderCombatants(SDL_Renderer* renderer) const {
    const SDL_FRect enemyPad{136.0f, 26.0f, 68.0f, 44.0f};
    const SDL_FRect playerPad{30.0f, 76.0f, 78.0f, 40.0f};
    drawFrame(renderer, enemyPad, SDL_Color{235, 235, 235, 255}, SDL_Color{158, 158, 158, 255});
    drawFrame(renderer, playerPad, SDL_Color{235, 235, 235, 255}, SDL_Color{158, 158, 158, 255});

    if (!wild_.spritePath.empty()) {
        if (SDL_Texture* wildTexture = textureManager_.load(wild_.spritePath.c_str())) {
            const SDL_FRect src{0.0f, 0.0f, 64.0f, 64.0f};
            const SDL_FRect dst{146.0f, 10.0f, 64.0f, 64.0f};
            SDL_RenderTexture(renderer, wildTexture, &src, &dst);
        }
    }

    if (!player_.spritePath.empty()) {
        if (SDL_Texture* playerTexture = textureManager_.load(player_.spritePath.c_str())) {
            const SDL_FRect src{0.0f, 0.0f, 64.0f, 64.0f};
            const SDL_FRect dst{24.0f, 52.0f, 64.0f, 64.0f};
            SDL_RenderTexture(renderer, playerTexture, &src, &dst);
        }
    }
}

void BattleScene::renderStatusPanels(SDL_Renderer* renderer) const {
    const SDL_FRect enemyPanel{8.0f, 8.0f, 118.0f, 34.0f};
    const SDL_FRect playerPanel{114.0f, 72.0f, 118.0f, 34.0f};
    drawFrame(renderer, enemyPanel, SDL_Color{247, 247, 247, 255}, SDL_Color{89, 95, 102, 255});
    drawFrame(renderer, playerPanel, SDL_Color{247, 247, 247, 255}, SDL_Color{89, 95, 102, 255});

    std::ostringstream wildLabel;
    wildLabel << "WILD " << wild_.name << " Lv" << wild_.level;
    renderLabel(renderer, enemyPanel.x + 4.0f, enemyPanel.y + 4.0f, wildLabel.str(), SDL_Color{24, 24, 24, 255});

    std::ostringstream playerLabel;
    playerLabel << player_.name << " Lv" << player_.level;
    renderLabel(renderer, playerPanel.x + 4.0f, playerPanel.y + 4.0f, playerLabel.str(), SDL_Color{24, 24, 24, 255});

    const float wildRatio = wild_.maxHp > 0 ? static_cast<float>(wild_.hp) / static_cast<float>(wild_.maxHp) : 0.0f;
    const float playerRatio = player_.maxHp > 0 ? static_cast<float>(player_.hp) / static_cast<float>(player_.maxHp) : 0.0f;
    drawHpBar(renderer, SDL_FRect{enemyPanel.x + 4.0f, enemyPanel.y + 20.0f, 72.0f, 8.0f}, wildRatio);
    drawHpBar(renderer, SDL_FRect{playerPanel.x + 4.0f, playerPanel.y + 20.0f, 72.0f, 8.0f}, playerRatio);

    std::ostringstream hpText;
    hpText << player_.hp << "/" << player_.maxHp;
    renderLabel(renderer, playerPanel.x + 80.0f, playerPanel.y + 20.0f, hpText.str(), SDL_Color{40, 40, 40, 255});
}

void BattleScene::renderMessageWindow(SDL_Renderer* renderer) const {
    const SDL_FRect messageRect{8.0f, 112.0f, 224.0f, 40.0f};
    drawFrame(renderer, messageRect, SDL_Color{248, 248, 248, 255}, SDL_Color{64, 74, 92, 255});

    std::string message = "What will " + player_.name + " do?";
    if (!messageQueue_.empty()) {
        message = messageQueue_.front();
    } else if (phase_ == Phase::Complete) {
        message = "Battle complete.";
    }

    if (message.size() <= 34) {
        renderLabel(renderer, messageRect.x + 6.0f, messageRect.y + 8.0f, message, SDL_Color{24, 24, 24, 255});
    } else {
        const std::size_t splitIndex = message.rfind(' ', 34);
        const std::size_t cut = (splitIndex == std::string::npos) ? 34 : splitIndex;
        renderLabel(renderer, messageRect.x + 6.0f, messageRect.y + 8.0f, message.substr(0, cut), SDL_Color{24, 24, 24, 255});
        renderLabel(
            renderer,
            messageRect.x + 6.0f,
            messageRect.y + 20.0f,
            message.substr(cut + ((cut < message.size() && message[cut] == ' ') ? 1 : 0)),
            SDL_Color{24, 24, 24, 255}
        );
    }
}

void BattleScene::renderCommandWindow(SDL_Renderer* renderer) const {
    if (phase_ != Phase::CommandSelect) {
        return;
    }

    const SDL_FRect commandRect{136.0f, 114.0f, 94.0f, 36.0f};
    drawFrame(renderer, commandRect, SDL_Color{238, 245, 255, 255}, SDL_Color{74, 94, 128, 255});

    for (int i = 0; i < kActionCount; ++i) {
        const int row = i / 2;
        const int col = i % 2;
        const float x = commandRect.x + 6.0f + (col * 44.0f);
        const float y = commandRect.y + 6.0f + (row * 14.0f);
        const SDL_Color textColor = (i == selectedAction_) ? SDL_Color{0, 0, 0, 255} : SDL_Color{48, 48, 48, 255};

        if (i == selectedAction_) {
            renderLabel(renderer, x, y, ">", SDL_Color{0, 0, 0, 255});
            renderLabel(renderer, x + 8.0f, y, actionLabel(i), textColor);
        } else {
            renderLabel(renderer, x + 8.0f, y, actionLabel(i), textColor);
        }
    }
}

void BattleScene::drawFrame(SDL_Renderer* renderer, const SDL_FRect& rect, const SDL_Color fill, const SDL_Color border) {
    SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
    SDL_RenderRect(renderer, &rect);
}

void BattleScene::drawHpBar(SDL_Renderer* renderer, const SDL_FRect& rect, const float ratio) {
    const float clamped = std::clamp(ratio, 0.0f, 1.0f);

    SDL_SetRenderDrawColor(renderer, 70, 70, 80, 255);
    SDL_RenderFillRect(renderer, &rect);

    SDL_Color fill{76, 214, 112, 255};
    if (clamped < 0.5f) {
        fill = SDL_Color{255, 214, 92, 255};
    }
    if (clamped < 0.2f) {
        fill = SDL_Color{238, 92, 92, 255};
    }

    SDL_FRect fillRect = rect;
    fillRect.w = std::max(1.0f, rect.w * clamped);
    SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRect(renderer, &fillRect);
}

void BattleScene::renderLabel(
    SDL_Renderer* renderer,
    const float x,
    const float y,
    const std::string& text,
    const SDL_Color color
) const {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDebugText(renderer, x, y, text.c_str());
}

const char* BattleScene::actionLabel(const int index) const {
    switch (index) {
    case static_cast<int>(Action::Attack):
        return "ATTACK";
    case static_cast<int>(Action::Dodge):
        return "DODGE";
    case static_cast<int>(Action::Block):
        return "BLOCK";
    case static_cast<int>(Action::Run):
        return "RUN";
    default:
        return "";
    }
}

