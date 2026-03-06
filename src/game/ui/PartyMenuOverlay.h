#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <SDL3/SDL_keycode.h>

#include "game/state/GameState.h"

class TextureManager;

enum class PartyMenuAction {
    None,
    Closed,
    OpenSummary
};

class PartyMenuOverlay {
public:
    bool isOpen() const { return open_; }
    int selectedIndex() const { return selectedIndex_; }
    void open(const std::vector<PartyPokemon>& party);
    void setSelectedIndex(int selectedIndex, const std::vector<PartyPokemon>& party);
    void close();
    PartyMenuAction handleKey(SDL_Keycode key, std::vector<PartyPokemon>& party);
    void render(TextureManager& textureManager, int viewportWidth, int viewportHeight, const std::vector<PartyPokemon>& party) const;

private:
    struct DisplayStats {
        int hp = 1;
        int maxHp = 1;
    };

    static constexpr int kMaxPartySlots = 6;

    int partyCount(const std::vector<PartyPokemon>& party) const;
    void moveSelection(SDL_Keycode key, int count);
    void setPrompt(std::string text);
    static DisplayStats makeDisplayStats(const PartyPokemon& member);
    static const char* speciesNameForId(int speciesId);
    static const char* speciesIconAssetPathForId(int speciesId);

    bool open_ = false;
    int selectedIndex_ = 0;
    int lastRightColumnIndex_ = 1;
    bool commandMenuOpen_ = false;
    int commandMenuIndex_ = 0;
    bool switchMode_ = false;
    int switchSourceIndex_ = 0;
    std::string promptText_ = "Choose a POKEMON.";
};
