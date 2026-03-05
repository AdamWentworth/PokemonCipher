#include "game/ui/DialogueOverlay.h"

#include <algorithm>
#include <sstream>
#include <vector>

#include <SDL3/SDL_render.h>

namespace {
std::vector<std::string> wrapDialogueText(const std::string& text, const std::size_t maxLineLength, const std::size_t maxLines) {
    std::vector<std::string> lines;
    if (maxLineLength == 0 || maxLines == 0) {
        return lines;
    }

    std::istringstream stream(text);
    std::string word;
    std::string currentLine;

    while (stream >> word) {
        const bool fits = currentLine.empty()
            ? word.size() <= maxLineLength
            : (currentLine.size() + 1 + word.size()) <= maxLineLength;

        if (fits) {
            if (!currentLine.empty()) {
                currentLine += ' ';
            }
            currentLine += word;
            continue;
        }

        if (!currentLine.empty()) {
            lines.push_back(currentLine);
            currentLine.clear();
            if (lines.size() >= maxLines) {
                return lines;
            }
        }

        if (word.size() <= maxLineLength) {
            currentLine = word;
        } else {
            std::size_t offset = 0;
            while (offset < word.size() && lines.size() < maxLines) {
                lines.push_back(word.substr(offset, maxLineLength));
                offset += maxLineLength;
            }
            if (lines.size() >= maxLines) {
                return lines;
            }
        }
    }

    if (!currentLine.empty() && lines.size() < maxLines) {
        lines.push_back(currentLine);
    }

    return lines;
}
} // namespace

void DialogueOverlay::show(const std::string& speaker, const std::string& text) {
    speaker_ = speaker;
    text_ = text;
    visible_ = true;
}

void DialogueOverlay::hide() {
    visible_ = false;
    speaker_.clear();
    text_.clear();
}

void DialogueOverlay::render(SDL_Renderer* renderer, const int viewportWidth, const int viewportHeight) const {
    if (!visible_ || !renderer) {
        return;
    }

    constexpr float kPadding = 6.0f;
    constexpr float kBoxHeight = 46.0f;
    SDL_FRect box{};
    box.x = kPadding;
    box.w = static_cast<float>(viewportWidth) - (kPadding * 2.0f);
    box.h = kBoxHeight;
    box.y = static_cast<float>(viewportHeight) - box.h - kPadding;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 20, 24, 32, 230);
    SDL_RenderFillRect(renderer, &box);

    SDL_SetRenderDrawColor(renderer, 232, 232, 232, 255);
    SDL_RenderRect(renderer, &box);

    if (!speaker_.empty()) {
        SDL_SetRenderDrawColor(renderer, 255, 231, 128, 255);
        SDL_RenderDebugText(renderer, box.x + 6.0f, box.y + 6.0f, speaker_.c_str());
    }

    SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
    const std::size_t maxCharsPerLine = static_cast<std::size_t>(std::max(8.0f, (box.w - 12.0f) / 8.0f));
    const std::vector<std::string> lines = wrapDialogueText(text_, maxCharsPerLine, 2);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const float y = box.y + 18.0f + (static_cast<float>(i) * 10.0f);
        SDL_RenderDebugText(renderer, box.x + 6.0f, y, lines[i].c_str());
    }

    SDL_RenderDebugText(renderer, box.x + box.w - 10.0f, box.y + box.h - 10.0f, ">");
}
