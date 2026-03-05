#pragma once

#include <cstddef>
#include <string>

struct SDL_Renderer;

class DialogueOverlay {
public:
    void show(const std::string& speaker, const std::string& text);
    void hide();
    void update(float dt, bool fastText);
    bool isVisible() const { return visible_; }
    bool isFullyRevealed() const;
    void revealAll();
    void render(SDL_Renderer* renderer, int viewportWidth, int viewportHeight) const;

private:
    bool visible_ = false;
    std::string speaker_;
    std::string fullText_;
    std::size_t revealedChars_ = 0;
    float revealAccumulator_ = 0.0f;
};
