#pragma once

#include <string>
#include <vector>

namespace OverworldDevConsoleParsing {
std::vector<std::string> splitTokens(const std::string& input);
std::string normalizeToken(std::string token);
std::vector<std::string> listLuaScriptIds();
bool parseBoolToken(const std::string& token, bool& valueOut);
bool parseIntToken(const std::string& token, int& valueOut);
bool parseFloatToken(const std::string& token, float& valueOut);
} // namespace OverworldDevConsoleParsing
