#pragma once

#include <vector>

#include <SDL3/SDL_keycode.h>

#include "game/state/GameState.h"

class TextureManager;
struct SDL_Renderer;
struct SDL_Texture;
struct PokemonSummaryLayout;
struct SpeciesDefinition;

enum class PokemonSummaryAction {
    None,
    Closed
};

class PokemonSummaryOverlay {
public:
    bool isOpen() const { return open_; }
    int selectedIndex() const { return selectedIndex_; }

    void open(const std::vector<PartyPokemon>& party, int selectedIndex);
    void close();
    PokemonSummaryAction handleKey(SDL_Keycode key, const std::vector<PartyPokemon>& party);
    void render(TextureManager& textureManager, int viewportWidth, int viewportHeight, const std::vector<PartyPokemon>& party) const;

private:
    struct DisplayStats {
        int hp = 1;
        int maxHp = 1;
        int attack = 1;
        int defense = 1;
        int speed = 1;
        int spAttack = 1;
        int spDefense = 1;
        int expPoints = 0;
        int expToNextLevel = 0;
        float hpRatio = 1.0f;
        float expRatio = 0.0f;
    };

    enum class SummaryPage {
        Info = 0,
        Skills = 1,
        Moves = 2
    };

    static constexpr int kMaxPartySlots = 6;

    int partyCount(const std::vector<PartyPokemon>& party) const;
    void moveMonSelection(SDL_Keycode key, int partyCount);
    void flipPage(SDL_Keycode key);
    void renderInfoPage(
        TextureManager& textureManager,
        SDL_Renderer* renderer,
        SDL_Texture* menuInfoTexture,
        const PartyPokemon& member,
        const char* speciesName,
        const SpeciesDefinition* speciesData,
        float scale,
        float offsetX,
        float offsetY,
        const PokemonSummaryLayout& layout
    ) const;
    void renderSkillsPage(
        TextureManager& textureManager,
        SDL_Renderer* renderer,
        const PartyPokemon& member,
        const DisplayStats& stats,
        float scale,
        float offsetX,
        float offsetY,
        const PokemonSummaryLayout& layout
    ) const;
    void renderMovesPage(
        TextureManager& textureManager,
        SDL_Renderer* renderer,
        SDL_Texture* menuInfoTexture,
        const PartyPokemon& member,
        const DisplayStats& stats,
        float scale,
        float offsetX,
        float offsetY,
        const PokemonSummaryLayout& layout
    ) const;

    static DisplayStats makeDisplayStats(const PartyPokemon& member);

    bool open_ = false;
    int selectedIndex_ = 0;
    SummaryPage page_ = SummaryPage::Info;
};
