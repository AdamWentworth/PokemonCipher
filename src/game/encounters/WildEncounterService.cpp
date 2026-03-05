#include "game/encounters/WildEncounterService.h"

#include <algorithm>
#include <cctype>

WildEncounterService::WildEncounterService() {
    EncounterTable route1Table{};
    route1Table.id = "route_1_grass";
    route1Table.slots = {
        {16, 2, 4, 55}, // Pidgey
        {19, 2, 4, 45}  // Rattata
    };

    EncounterTable defaultGrassTable{};
    defaultGrassTable.id = "default_grass";
    defaultGrassTable.slots = {
        {16, 2, 3, 50},
        {19, 2, 3, 50}
    };

    tables_.push_back(route1Table);
    tables_.push_back(defaultGrassTable);
}

std::optional<WildEncounter> WildEncounterService::rollEncounter(const std::string& tableId, std::mt19937& rng) const {
    const EncounterTable* table = findTable(tableId);
    if (!table) {
        table = findTable("default_grass");
    }

    if (!table || table->slots.empty()) {
        return std::nullopt;
    }

    int totalWeight = 0;
    for (const EncounterSlot& slot : table->slots) {
        totalWeight += std::max(1, slot.weight);
    }

    if (totalWeight <= 0) {
        return std::nullopt;
    }

    std::uniform_int_distribution<int> slotRoll(1, totalWeight);
    int roll = slotRoll(rng);

    const EncounterSlot* selectedSlot = &table->slots.front();
    for (const EncounterSlot& slot : table->slots) {
        roll -= std::max(1, slot.weight);
        if (roll <= 0) {
            selectedSlot = &slot;
            break;
        }
    }

    int minLevel = std::min(selectedSlot->minLevel, selectedSlot->maxLevel);
    int maxLevel = std::max(selectedSlot->minLevel, selectedSlot->maxLevel);
    minLevel = std::max(1, minLevel);
    maxLevel = std::max(minLevel, maxLevel);

    std::uniform_int_distribution<int> levelRoll(minLevel, maxLevel);

    WildEncounter encounter{};
    encounter.tableId = table->id;
    encounter.speciesId = selectedSlot->speciesId;
    encounter.speciesName = speciesNameForId(selectedSlot->speciesId);
    encounter.level = levelRoll(rng);
    return encounter;
}

std::vector<std::string> WildEncounterService::tableIds() const {
    std::vector<std::string> ids;
    ids.reserve(tables_.size());
    for (const EncounterTable& table : tables_) {
        ids.push_back(table.id);
    }

    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

std::string WildEncounterService::normalizeKey(std::string key) {
    std::transform(key.begin(), key.end(), key.begin(), [](const unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return key;
}

std::string WildEncounterService::speciesNameForId(const int speciesId) {
    switch (speciesId) {
    case 16:
        return "Pidgey";
    case 19:
        return "Rattata";
    case 133:
        return "Eevee";
    default:
        return "Pokemon";
    }
}

const WildEncounterService::EncounterTable* WildEncounterService::findTable(const std::string& tableId) const {
    const std::string normalized = normalizeKey(tableId);
    for (const EncounterTable& table : tables_) {
        if (normalizeKey(table.id) == normalized) {
            return &table;
        }
    }

    return nullptr;
}
