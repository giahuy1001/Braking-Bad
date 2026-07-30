#include "CDeer.h"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <cmath>

static sf::Texture& getDeerTexture(direction dir) {
    static sf::Texture tRight, tLeft;
    static bool loaded = false;
    if (!loaded) {
        tRight.loadFromFile("assets/obstacles/Huou1.png");
        tLeft.loadFromFile("assets/obstacles/Huou2.png");
        loaded = true;
    }
    return (dir == RIGHT) ? tRight : tLeft;
}

// Larger bounding box for the deer and a slightly higher base speed
CDeer::CDeer(float startX, float startY, direction dir)
    : CAnimal(startX, startY, 60.0f, 60.0f, 150.0f, dir), leapCycle(0.0f)
    //x, y, width, height, speed, direction, leapCycle
{

    sf::Texture& tex = getDeerTexture(dir);
    sprite = new sf::Sprite(tex);

    // 6 cột, 1 hàng = 6 frames
    totalFrames = 6;
    frameCols = 6;
    frameWidth = static_cast<int>(tex.getSize().x / 6);
    frameHeight = static_cast<int>(tex.getSize().y);

    sprite->setTextureRect(sf::IntRect({ 0, 0 }, { frameWidth, frameHeight }));
    sprite->setScale({ width / static_cast<float>(frameWidth), height / static_cast<float>(frameHeight) });
    frameDuration = 0.1f;
}

void CDeer::move(float dt) {
    leapCycle += dt * 4.0f;

    float leapMultiplier = std::abs(std::sin(leapCycle)) * 2.5f;

    // Apply BOTH the leap math AND the Endless mode scaling
    float currentSpeed = (speed + (speed * leapMultiplier)) * speedMultiplier;

    if (dir == RIGHT) {
        x += currentSpeed * dt;
    }
    else {
        x -= currentSpeed * dt;
    }

    updateAnimation(dt);
}