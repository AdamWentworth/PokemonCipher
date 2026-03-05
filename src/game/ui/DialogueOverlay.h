#pragma once

#include <string>

struct SDL_Renderer;

class DialogueOverlay {
public:
    void show(const std::string& speaker, const std::string& text);
    void hide();
    void render(SDL_Renderer* renderer, int viewportWidth, int viewportHeight) const;

private:
    bool visible_ = false;
    std::string speaker_;
    std::string text_;
};
