#include "game/ui/SummaryContent.h"

#include <algorithm>
#include <cstddef>

#include "game/data/SpeciesRegistry.h"

namespace {
constexpr int kTypeMystery = 9;
} // namespace

namespace summary_content {
const char* pageTitleFor(const int pageIndex) {
    switch (pageIndex) {
    case 0:
        return "POKEMON INFO";
    case 1:
        return "POKEMON SKILLS";
    case 2:
        return "KNOWN MOVES";
    default:
        return "POKEMON INFO";
    }
}

const char* controlsTextFor(const int pageIndex) {
    (void)pageIndex;
    return "";
}

const char* itemTextFor(const PartyPokemon& member) {
    if (member.isPartner) {
        return "NONE";
    }
    return "NONE";
}

const SpeciesDefinition* speciesDefinitionFor(const PartyPokemon& member) {
    return getSpeciesRegistry().find(member.speciesId);
}

const char* abilityNameFor(const PartyPokemon& member) {
    if (const SpeciesDefinition* species = speciesDefinitionFor(member);
        species != nullptr && !species->abilityName.empty()) {
        return species->abilityName.c_str();
    }
    return "UNKNOWN";
}

const char* abilityDescriptionFor(const PartyPokemon& member) {
    if (const SpeciesDefinition* species = speciesDefinitionFor(member);
        species != nullptr && !species->abilityDescription.empty()) {
        return species->abilityDescription.c_str();
    }
    return "No ability data.";
}

SpeciesTypeDisplay typeIdsForSpecies(const PartyPokemon& member) {
    if (const SpeciesDefinition* species = speciesDefinitionFor(member)) {
        return SpeciesTypeDisplay{species->primaryTypeId, species->secondaryTypeId};
    }
    return SpeciesTypeDisplay{kTypeMystery, kTypeMystery};
}

int movePpColorIndex(const int pp, const int maxPp) {
    if (maxPp <= 0 || pp >= maxPp) {
        return 0;
    }

    if (pp <= 0) {
        return 3;
    }

    if (maxPp == 3) {
        if (pp == 2) {
            return 2;
        }
        if (pp == 1) {
            return 1;
        }
    } else if (maxPp == 2) {
        if (pp == 1) {
            return 1;
        }
    } else {
        if (pp <= (maxPp / 4)) {
            return 2;
        }
        if (pp <= (maxPp / 2)) {
            return 1;
        }
    }

    return 0;
}

std::array<MoveDisplay, 4> movesFor(const PartyPokemon& member) {
    std::array<MoveDisplay, 4> result{{
        {"-------", 0, 0, kTypeMystery},
        {"-------", 0, 0, kTypeMystery},
        {"-------", 0, 0, kTypeMystery},
        {"-------", 0, 0, kTypeMystery},
    }};

    const SpeciesDefinition* species = speciesDefinitionFor(member);
    if (!species) {
        return result;
    }

    for (std::size_t i = 0; i < result.size(); ++i) {
        const SpeciesMoveDefinition& move = species->moves[i];
        result[i].name = move.name.empty() ? "-------" : move.name.c_str();
        result[i].pp = std::max(0, move.pp);
        result[i].maxPp = std::max(0, move.maxPp);
        result[i].typeId = move.typeId;
    }
    return result;
}
} // namespace summary_content
