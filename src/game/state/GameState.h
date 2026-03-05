#pragma once

#include <cctype>
#include <string>
#include <unordered_map>

class GameState {
public:
    void setFlag(const std::string& key, const bool value) {
        flags_[normalize(key)] = value;
    }

    bool getFlag(const std::string& key, const bool defaultValue = false) const {
        const auto it = flags_.find(normalize(key));
        if (it == flags_.end()) {
            return defaultValue;
        }

        return it->second;
    }

    void setVar(const std::string& key, const int value) {
        vars_[normalize(key)] = value;
    }

    int getVar(const std::string& key, const int defaultValue = 0) const {
        const auto it = vars_.find(normalize(key));
        if (it == vars_.end()) {
            return defaultValue;
        }

        return it->second;
    }

private:
    static std::string normalize(const std::string& key) {
        std::string normalized;
        normalized.reserve(key.size());
        for (const char ch : key) {
            normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }
        return normalized;
    }

    std::unordered_map<std::string, bool> flags_;
    std::unordered_map<std::string, int> vars_;
};
