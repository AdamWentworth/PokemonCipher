#include "game/dev/OverworldDevConsoleHandlers.h"

#include <sstream>
#include <string>
#include <vector>

#include "game/dev/OverworldDevConsoleParsing.h"
#include "game/state/GameState.h"

namespace OverworldDevConsoleHandlers {
bool handleStateCommand(
    const std::string& command,
    const std::vector<std::string>& tokens,
    const OverworldDevConsole::Dependencies& dependencies
) {
    if (command == "setflag") {
        if (!dependencies.print) {
            return true;
        }

        if (tokens.size() != 3) {
            dependencies.print("Usage: setflag <key> <0|1|true|false>");
            return true;
        }

        if (!dependencies.gameState) {
            dependencies.print("Game state unavailable.");
            return true;
        }

        bool flagValue = false;
        if (!OverworldDevConsoleParsing::parseBoolToken(tokens[2], flagValue)) {
            dependencies.print("setflag expects 0/1/true/false/on/off.");
            return true;
        }

        dependencies.gameState->setFlag(tokens[1], flagValue);
        dependencies.print("Flag set: " + tokens[1]);
        return true;
    }

    if (command == "getflag") {
        if (!dependencies.print) {
            return true;
        }

        if (tokens.size() != 2) {
            dependencies.print("Usage: getflag <key>");
            return true;
        }

        if (!dependencies.gameState) {
            dependencies.print("Game state unavailable.");
            return true;
        }

        dependencies.print("Flag " + tokens[1] + " = " + (dependencies.gameState->getFlag(tokens[1]) ? "true" : "false"));
        return true;
    }

    if (command == "setvar") {
        if (!dependencies.print) {
            return true;
        }

        if (tokens.size() != 3) {
            dependencies.print("Usage: setvar <key> <int>");
            return true;
        }

        if (!dependencies.gameState) {
            dependencies.print("Game state unavailable.");
            return true;
        }

        int value = 0;
        if (!OverworldDevConsoleParsing::parseIntToken(tokens[2], value)) {
            dependencies.print("setvar expects an integer value.");
            return true;
        }

        dependencies.gameState->setVar(tokens[1], value);
        dependencies.print("Var set: " + tokens[1]);
        return true;
    }

    if (command == "getvar") {
        if (!dependencies.print) {
            return true;
        }

        if (tokens.size() != 2) {
            dependencies.print("Usage: getvar <key>");
            return true;
        }

        if (!dependencies.gameState) {
            dependencies.print("Game state unavailable.");
            return true;
        }

        const int value = dependencies.gameState->getVar(tokens[1]);
        std::ostringstream stream;
        stream << "Var " << tokens[1] << " = " << value;
        dependencies.print(stream.str());
        return true;
    }

    return false;
}
} // namespace OverworldDevConsoleHandlers
