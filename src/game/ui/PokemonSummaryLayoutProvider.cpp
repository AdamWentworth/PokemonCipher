#include "game/ui/PokemonSummaryLayoutProvider.h"

#include <iostream>
#include <string>

#include "game/ui/PokemonSummaryLayout.h"

const PokemonSummaryLayout& pokemonSummaryLayout() {
    static const PokemonSummaryLayout layout = []() {
        PokemonSummaryLayout loaded = makeDefaultPokemonSummaryLayout();
        std::string error;
        if (!loadPokemonSummaryLayoutFromLuaFile("assets/config/ui/pokemon_summary_layout.lua", loaded, error) &&
            !error.empty()) {
            std::cout << "Summary layout config load failed; using defaults: " << error << '\n';
        }
        return loaded;
    }();
    return layout;
}

