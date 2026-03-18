#ifndef ASSIGNMENT2_GAMEOBJECT_H
#define ASSIGNMENT2_GAMEOBJECT_H

#include "Game.h"

class GameObject {
public:
    GameObject(const char* path, int x, int y);
    ~GameObject();

    void update(float deltaTime);
    void draw();

private:
    float xPos{}, yPos{};
    bool running;

    SDL_Texture* texture = nullptr;
    SDL_FRect srcRect{}, dstRect{};
};

#endif //ASSIGNMENT2_GAMEOBJECT_H