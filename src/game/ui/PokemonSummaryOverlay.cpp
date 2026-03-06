#include "game/ui/PokemonSummaryOverlay.h"

#include <algorithm>

void PokemonSummaryOverlay::open(const std::vector<PartyPokemon>& party, const int selectedIndex) {
    const int count = partyCount(party);
    if (count <= 0) {
        open_ = false;
        selectedIndex_ = 0;
        return;
    }

    open_ = true;
    selectedIndex_ = std::clamp(selectedIndex, 0, count - 1);
    page_ = SummaryPage::Info;
}

void PokemonSummaryOverlay::close() {
    open_ = false;
}

PokemonSummaryAction PokemonSummaryOverlay::handleKey(const SDL_Keycode key, const std::vector<PartyPokemon>& party) {
    if (!open_) {
        return PokemonSummaryAction::None;
    }

    if (key == SDLK_ESCAPE || key == SDLK_X || key == SDLK_BACKSPACE) {
        close();
        return PokemonSummaryAction::Closed;
    }

    const int count = partyCount(party);
    if (count <= 0) {
        selectedIndex_ = 0;
        return PokemonSummaryAction::None;
    }

    if (key == SDLK_LEFT || key == SDLK_A || key == SDLK_RIGHT || key == SDLK_D) {
        flipPage(key);
        return PokemonSummaryAction::None;
    }

    if (key == SDLK_UP || key == SDLK_W || key == SDLK_DOWN || key == SDLK_S) {
        moveMonSelection(key, count);
    }

    return PokemonSummaryAction::None;
}

int PokemonSummaryOverlay::partyCount(const std::vector<PartyPokemon>& party) const {
    return std::min<int>(kMaxPartySlots, static_cast<int>(party.size()));
}

void PokemonSummaryOverlay::moveMonSelection(const SDL_Keycode key, const int count) {
    if (count <= 1) {
        selectedIndex_ = 0;
        return;
    }

    if (key == SDLK_UP || key == SDLK_W) {
        selectedIndex_ = (selectedIndex_ == 0) ? (count - 1) : (selectedIndex_ - 1);
    } else if (key == SDLK_DOWN || key == SDLK_S) {
        selectedIndex_ = (selectedIndex_ + 1) % count;
    }
}

void PokemonSummaryOverlay::flipPage(const SDL_Keycode key) {
    if (key == SDLK_LEFT || key == SDLK_A) {
        if (page_ == SummaryPage::Info) {
            page_ = SummaryPage::Moves;
        } else if (page_ == SummaryPage::Skills) {
            page_ = SummaryPage::Info;
        } else {
            page_ = SummaryPage::Skills;
        }
    } else if (key == SDLK_RIGHT || key == SDLK_D) {
        if (page_ == SummaryPage::Info) {
            page_ = SummaryPage::Skills;
        } else if (page_ == SummaryPage::Skills) {
            page_ = SummaryPage::Moves;
        } else {
            page_ = SummaryPage::Info;
        }
    }
}

PokemonSummaryOverlay::DisplayStats PokemonSummaryOverlay::makeDisplayStats(const PartyPokemon& member) {
    DisplayStats stats{};
    stats.maxHp = std::max(1, 12 + (member.level * 3));
    stats.hp = stats.maxHp;
    stats.attack = std::max(1, 5 + (member.level * 2));
    stats.defense = std::max(1, 5 + (member.level * 2));
    stats.speed = std::max(1, 5 + (member.level * 2));
    stats.spAttack = std::max(1, 5 + (member.level * 2));
    stats.spDefense = std::max(1, 5 + (member.level * 2));
    stats.expPoints = member.level * member.level * member.level;

    const int nextLevel = std::min(100, member.level + 1);
    const int nextLevelExp = nextLevel * nextLevel * nextLevel;
    stats.expToNextLevel = std::max(0, nextLevelExp - stats.expPoints);

    const int currentLevelBase = member.level * member.level * member.level;
    const int currentLevelCap = nextLevelExp;
    const int levelSpan = std::max(1, currentLevelCap - currentLevelBase);
    stats.expRatio = std::clamp(
        static_cast<float>(stats.expPoints - currentLevelBase) / static_cast<float>(levelSpan),
        0.0f,
        1.0f
    );
    stats.hpRatio = std::clamp(static_cast<float>(stats.hp) / static_cast<float>(stats.maxHp), 0.0f, 1.0f);
    return stats;
}

