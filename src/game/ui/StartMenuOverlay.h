#pragma once

#include <string>

#include <SDL3/SDL_keycode.h>

struct SDL_Renderer;

enum class StartMenuAction {
    None,
    Closed,
    Party,
    Bag,
    Save,
    Options
};

class StartMenuOverlay {
public:
    bool isOpen() const { return open_; }
    void open();
    void close();
    StartMenuAction handleKey(SDL_Keycode key);
    void setStatusText(std::string text);
    void clearStatusText();
    void render(SDL_Renderer* renderer, int viewportWidth, int viewportHeight) const;

private:
    static constexpr int kEntryCount = 5;
    static const char* entryLabel(int index);

    bool open_ = false;
    int selectedIndex_ = 0;
    std::string statusText_;
};
