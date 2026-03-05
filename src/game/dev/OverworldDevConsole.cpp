#include "game/dev/OverworldDevConsole.h"

#include <utility>

#include "game/dev/OverworldDevConsoleHandlers.h"
#include "game/dev/OverworldDevConsoleParsing.h"

OverworldDevConsole::OverworldDevConsole(Dependencies dependencies) : dependencies_(std::move(dependencies)) {}

void OverworldDevConsole::execute(const std::string& commandLine) const {
    if (!dependencies_.print) {
        return;
    }

    const std::vector<std::string> tokens = OverworldDevConsoleParsing::splitTokens(commandLine);
    if (tokens.empty()) {
        return;
    }

    const std::string command = OverworldDevConsoleParsing::normalizeToken(tokens[0]);

    if (OverworldDevConsoleHandlers::handleInfoCommand(command, tokens, dependencies_)) {
        return;
    }

    if (OverworldDevConsoleHandlers::handleScriptCommand(command, tokens, dependencies_)) {
        return;
    }

    if (OverworldDevConsoleHandlers::handleEncounterCommand(command, tokens, dependencies_)) {
        return;
    }

    if (OverworldDevConsoleHandlers::handleTravelCommand(command, tokens, dependencies_)) {
        return;
    }

    if (OverworldDevConsoleHandlers::handleStateCommand(command, tokens, dependencies_)) {
        return;
    }

    dependencies_.print("Unknown command: " + tokens[0]);
}
