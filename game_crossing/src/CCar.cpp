#include "CCar.h"
#include <SFML/Graphics.hpp>

static sf::Texture& getCarTexture(direction dir) {
    static sf::Texture tRight, tLeft;
    static bool loaded = false;
    if (!loaded) {
        // 1: Trái sang phải (RIGHT), 2: Phải sang trái (LEFT)
        tRight.loadFromFile("assets/obstacles/Car1.png");
        tLeft.loadFromFile("assets/obstacles/Car2.png");
        loaded = true;
    }
    return (dir == RIGHT) ? tRight : tLeft;
}

CCar::CCar(float startX, float startY, direction dir)
    : CVehicle(startX, startY, 80.0f, 50.0f, 350.0f, dir) {

    sf::Texture& tex = getCarTexture(dir);
    sprite = new sf::Sprite(tex);

    // Cắt sprite sheet (4 cột, 3 hàng = 12 frames)
    totalFrames = 1;
    frameCols = 1;
    frameWidth = static_cast<int>(tex.getSize().x);
    frameHeight = static_cast<int>(tex.getSize().y);

    //sprite->setTextureRect(sf::IntRect({ 0, 0 }, { frameWidth, frameHeight }));
    sprite->setScale({ width / static_cast<float>(frameWidth), height / static_cast<float>(frameHeight) });
    frameDuration = 0.05f; // Tốc độ bánh xe lăn
}

void CCar::move(float dt) {
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

    //updateAnimation(dt);
}