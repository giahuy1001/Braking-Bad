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
// 1. CHỈNH HITBOX TẠI ĐÂY:
// Mình đang ví dụ thu nhỏ xuống còn 150.0f (rộng) và 100.0f (cao). Bạn có thể đổi số này tuỳ ý.
    : CVehicle(startX, startY, 150.0f, 75.0f, 400.0f, dir) {

    sf::Texture& tex = getTruckTexture(dir);
    sprite = new sf::Sprite(tex);

    totalFrames = 12;
    frameCols = 4;
    frameWidth = static_cast<int>(tex.getSize().x / 4);
    frameHeight = static_cast<int>(tex.getSize().y / 3);

    sprite->setTextureRect(sf::IntRect({ 0, 0 }, { frameWidth, frameHeight }));

    // 2. GIỮ NGUYÊN KÍCH THƯỚC ẢNH GỐC:
    // Thay chữ 'width' và 'height' bằng đúng con số gốc của bạn là 200.0f và 160.0f
    float visualWidth = 200.0f;
    float visualHeight = 160.0f;
    sprite->setScale({ visualWidth / static_cast<float>(frameWidth), visualHeight / static_cast<float>(frameHeight) });

    // 3. ĐƯA HITBOX VÀO CHÍNH GIỮA ẢNH XE:
    // Công thức dời tâm: (Kích thước ảnh gốc - Kích thước Hitbox mới) chia 2
    // X = (200.0f - 150.0f) / 2 = 25.0f
    // Y = (160.0f - 100.0f) / 2 = 30.0f
    float scaleX = visualWidth / static_cast<float>(frameWidth);
    float scaleY = visualHeight / static_cast<float>(frameHeight);
    sprite->setOrigin({ 25.0f / scaleX, 30.0f / scaleY });

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