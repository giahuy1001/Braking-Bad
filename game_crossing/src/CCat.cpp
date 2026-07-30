#include "CCat.h"
#include <SFML/Graphics.hpp>
#include <iostream>

static sf::Texture& getCatTexture(direction dir) {
    static sf::Texture tRight, tLeft;
    static bool loaded = false;
    if (!loaded) {
        tRight.loadFromFile("assets/obstacles/Cat1.png");
        tLeft.loadFromFile("assets/obstacles/Cat2.png");
        loaded = true;
    }
    return (dir == RIGHT) ? tRight : tLeft;
}

// Standard bounding box and moderate base speed
CCat::CCat(float startX, float startY, direction dir)
    : CAnimal(startX, startY, 40.0f, 40.0f, 200.0f, dir), burstTimer(0.0f), isBursting(false) {

    sf::Texture& tex = getCatTexture(dir);
    sprite = new sf::Sprite(tex);

    // 8 cột, 1 hàng = 8 frames
    totalFrames = 8;
    frameCols = 8;
    frameWidth = static_cast<int>(tex.getSize().x / 8);
    frameHeight = static_cast<int>(tex.getSize().y);

    sprite->setTextureRect(sf::IntRect({ 0, 0 }, { frameWidth, frameHeight }));
    sprite->setScale({ width / static_cast<float>(frameWidth), height / static_cast<float>(frameHeight) });
    frameDuration = 0.08f;
}

void CCat::move(float dt) {
    burstTimer += dt;

    if (burstTimer > 1.5f) {
        isBursting = !isBursting;
        burstTimer = 0.0f;
    }

    // Apply BOTH the burst behavior AND the Endless mode scaling
    float currentSpeed = (isBursting ? (speed * 3.0f) : speed) * speedMultiplier;

    if (dir == RIGHT) {
        x += currentSpeed * dt;
    }
    else {
        x -= currentSpeed * dt;
    }

    // Chạy nhanh thì animation đạp chân cũng nhanh theo
    frameDuration = isBursting ? 0.04f : 0.08f;
    updateAnimation(dt);
}