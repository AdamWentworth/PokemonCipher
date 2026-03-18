#include "GameObject.h"
#include "TextureManager.h"
#include "Game.h"
#include <chrono>
#include <iostream>
#include <ostream>


GameObject::GameObject(const char *path, int x, int y) :
    xPos(x), yPos(y), texture(TextureManager::load(path))
{}

GameObject::~GameObject() {
    if (texture) SDL_DestroyTexture(texture);
}

void GameObject::update(float deltaTime) {

    float speed = 300.0f;

    // Move gameobject one pixel on the x and y each frame
    // xPos++;
    // yPos++;
    xPos += speed * deltaTime;
    yPos += speed * deltaTime;

    int ticks{};
    float deltaTimeSec{};

    // while (running) {
    //     int currentTicks = SDL_GetTicks();
    //     deltaTimeSec = ( currentTicks - ticks ) / 1000.0f;
    //     ticks = currentTicks;
    //     std::cout << "ticks: " << ticks << std::endl;
    //
    //     xPos += deltaTimeSec;
    //     yPos += deltaTimeSec;
    // }

    // Where do we want to crop the texture from?
    srcRect.x = srcRect.y = 0.5;

    // How much to crop?
    srcRect.w = srcRect.h = 55;

    // Where to draw?
    dstRect.x = static_cast<float>(xPos);
    dstRect.y = static_cast<float>(yPos);

    // What sizes to render?
    dstRect.h = srcRect.h;
    dstRect.w = srcRect.w;
}

void GameObject::draw() {
    TextureManager::draw(texture, srcRect, dstRect);
}
