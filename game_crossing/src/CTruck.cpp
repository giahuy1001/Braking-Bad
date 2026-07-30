#include "CTruck.h"
#include <SFML/Graphics.hpp>

static sf::Texture& getTruckTexture(direction dir) {
    static sf::Texture tRight, tLeft;
    static bool loaded = false;
    if (!loaded) {
        tRight.loadFromFile("assets/obstacles/Ambulance1.png");
        tLeft.loadFromFile("assets/obstacles/Ambulance2.png");
        loaded = true;
    }
    return (dir == RIGHT) ? tRight : tLeft;
}

// Implements a larger bounding box (160x60) and slower velocity (6.0f)
CTruck::CTruck(float startX, float startY, direction dir)
    : CVehicle(startX, startY, 160.0f, 60.0f, 300.0f, dir) {

    sf::Texture& tex = getTruckTexture(dir);
    sprite = new sf::Sprite(tex);

    totalFrames = 12;
    frameCols = 4;
    frameWidth = static_cast<int>(tex.getSize().x / 4);
    frameHeight = static_cast<int>(tex.getSize().y / 3);

    sprite->setTextureRect(sf::IntRect({ 0, 0 }, { frameWidth, frameHeight }));
    sprite->setScale({ width / static_cast<float>(frameWidth), height / static_cast<float>(frameHeight) });
    frameDuration = 0.08f;
}

void CTruck::move(float dt) {
    if (getStoppingStatus()) {
        return;
    }

    // Factor in the Endless Mode multiplier
    float currentSpeed = speed * speedMultiplier;

    if (dir == RIGHT) {
        x += currentSpeed * dt;
    }
    else {
        x -= currentSpeed * dt;
    }

    updateAnimation(dt);
}