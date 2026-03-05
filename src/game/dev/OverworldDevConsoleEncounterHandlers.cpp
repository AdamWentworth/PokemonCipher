#include "game/dev/OverworldDevConsoleHandlers.h"

#include <string>
#include <vector>

namespace OverworldDevConsoleHandlers {
bool handleEncounterCommand(
    const std::string& command,
    const std::vector<std::string>& tokens,
    const OverworldDevConsole::Dependencies& dependencies
) {
    if (command == "encounters") {
        if (!dependencies.print) {
            return true;
        }

        if (!dependencies.listEncounterTableIds) {
            dependencies.print("Encounter tables unavailable.");
            return true;
        }

        const std::vector<std::string> tableIds = dependencies.listEncounterTableIds();
        if (tableIds.empty()) {
            dependencies.print("No encounter tables available.");
            return true;
        }

        std::string line = "Encounter tables:";
        for (const std::string& id : tableIds) {
            line += " " + id;
        }
        dependencies.print(line);
        return true;
    }

    if (command == "encounter") {
        if (!dependencies.print) {
            return true;
        }

        if (tokens.size() > 2) {
            dependencies.print("Usage: encounter [tableId]");
            return true;
        }

        if (!dependencies.triggerEncounter) {
            dependencies.print("Encounter trigger unavailable.");
            return true;
        }

        const std::string tableId = tokens.size() == 2 ? tokens[1] : "";
        if (!dependencies.triggerEncounter(tableId)) {
            dependencies.print("Encounter trigger failed.");
            return true;
        }

        if (tableId.empty()) {
            dependencies.print("Encounter triggered from current zone.");
        } else {
            dependencies.print("Encounter triggered using table: " + tableId);
        }
        return true;
    }

    return false;
}
} // namespace OverworldDevConsoleHandlers
