#pragma once

#include <array>

#include "game/state/GameState.h"

struct SpeciesDefinition;

namespace summary_content {
struct MoveDisplay {
    const char* name = "-------";
    int pp = 0;
    int maxPp = 0;
    int typeId = 9;
};

struct SpeciesTypeDisplay {
    int primary = 9;
    int secondary = 9;
};

const char* pageTitleFor(int pageIndex);
const char* controlsTextFor(int pageIndex);
const char* itemTextFor(const PartyPokemon& member);
const SpeciesDefinition* speciesDefinitionFor(const PartyPokemon& member);
const char* abilityNameFor(const PartyPokemon& member);
const char* abilityDescriptionFor(const PartyPokemon& member);
SpeciesTypeDisplay typeIdsForSpecies(const PartyPokemon& member);
std::array<MoveDisplay, 4> movesFor(const PartyPokemon& member);
int movePpColorIndex(int pp, int maxPp);
} // namespace summary_content
