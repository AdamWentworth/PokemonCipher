#pragma once

#include <array>
#include <string>
#include <unordered_map>

struct SpeciesMoveDefinition {
    std::string name = "-------";
    int pp = 0;
    int maxPp = 0;
    int typeId = 9;
};

struct SpeciesDefinition {
    int id = 0;
    std::string name = "POKEMON";
    int primaryTypeId = 9;
    int secondaryTypeId = 9;
    std::string abilityName = "UNKNOWN";
    std::string abilityDescription = "No ability data.";
    std::string frontSpritePath;
    std::string iconPath;
    std::array<SpeciesMoveDefinition, 4> moves{};
};

class SpeciesRegistry {
public:
    void addSpecies(const SpeciesDefinition& species);
    const SpeciesDefinition* find(int speciesId) const;

private:
    std::unordered_map<int, SpeciesDefinition> byId_;
};

SpeciesRegistry buildFallbackSpeciesRegistry();
bool loadSpeciesRegistryFromLuaFile(const std::string& scriptPath, SpeciesRegistry& outRegistry, std::string& errorOut);
const SpeciesRegistry& getSpeciesRegistry();
