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
    Options,
    ToggleTextSpeed,
    ToggleBattleStyle
};

class StartMenuOverlay {
public:
    bool isOpen() const { return open_; }
    bool isOptionsOpen() const { return optionsOpen_; }
    void open();
    void close();
    StartMenuAction handleKey(SDL_Keycode key);
    void openOptions(bool textSpeedFast, bool battleStyleSet);
    void syncOptions(bool textSpeedFast, bool battleStyleSet);
    void setStatusText(std::string text);
    void clearStatusText();
    void render(SDL_Renderer* renderer, int viewportWidth, int viewportHeight) const;

private:
    static constexpr int kMainEntryCount = 5;
    static constexpr int kOptionsEntryCount = 3;
    static const char* entryLabel(int index);
    static const char* optionsEntryLabel(int index, bool textSpeedFast, bool battleStyleSet);

    bool open_ = false;
    bool optionsOpen_ = false;
    int selectedIndex_ = 0;
    int optionsSelectedIndex_ = 0;
    bool textSpeedFast_ = false;
    bool battleStyleSet_ = false;
    std::string statusText_;
};
