#include "game/dev/OverworldDevConsoleParsing.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <vector>

namespace OverworldDevConsoleParsing {
std::vector<std::string> splitTokens(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream stream(input);
    std::string token;

    while (stream >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

std::string normalizeToken(std::string token) {
    std::transform(token.begin(), token.end(), token.begin(), [](const unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return token;
}

std::vector<std::string> listLuaScriptIds() {
    std::vector<std::string> scriptIds;
    const std::filesystem::path scriptsDirectory = std::filesystem::path("assets") / "scripts";
    if (!std::filesystem::exists(scriptsDirectory) || !std::filesystem::is_directory(scriptsDirectory)) {
        return scriptIds;
    }

    for (const auto& entry : std::filesystem::directory_iterator(scriptsDirectory)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const std::filesystem::path path = entry.path();
        if (path.extension() != ".lua") {
            continue;
        }

        scriptIds.push_back(path.stem().string());
    }

    std::sort(scriptIds.begin(), scriptIds.end());
    scriptIds.erase(std::unique(scriptIds.begin(), scriptIds.end()), scriptIds.end());
    return scriptIds;
}

bool parseBoolToken(const std::string& token, bool& valueOut) {
    std::string normalized = normalizeToken(token);
    if (normalized == "1" || normalized == "true" || normalized == "on" || normalized == "yes") {
        valueOut = true;
        return true;
    }

    if (normalized == "0" || normalized == "false" || normalized == "off" || normalized == "no") {
        valueOut = false;
        return true;
    }

    return false;
}

bool parseIntToken(const std::string& token, int& valueOut) {
    try {
        std::size_t parsedChars = 0;
        const int value = std::stoi(token, &parsedChars);
        if (parsedChars != token.size()) {
            return false;
        }

        valueOut = value;
        return true;
    } catch (...) {
        return false;
    }
}

bool parseFloatToken(const std::string& token, float& valueOut) {
    try {
        std::size_t parsedChars = 0;
        const float value = std::stof(token, &parsedChars);
        if (parsedChars != token.size()) {
            return false;
        }

        valueOut = value;
        return true;
    } catch (...) {
        return false;
    }
}
} // namespace OverworldDevConsoleParsing
