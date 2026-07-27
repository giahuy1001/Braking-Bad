#include "../include/CGameObject.h"

CGameObject::CGameObject(float startX, float startY, float w, float h) 
    : x{startX}, y{startY}, width{w}, height{h}, sprite{nullptr}
{}

CGameObject::~CGameObject() {
    if (sprite != nullptr) {
        delete sprite;
        sprite = nullptr;
    }
}

void CGameObject::draw(sf::RenderWindow& window, float cameraY) {
    if (sprite != nullptr) {
        sprite->setPosition({ x, y - cameraY });
        window.draw(*sprite);
    }
    else {
        // DEBUG FALLBACK: Draw a bright red box for obstacles
        sf::RectangleShape debugBox({ width, height });
        debugBox.setPosition({ x, y - cameraY });
        debugBox.setFillColor(sf::Color::Red);
        window.draw(debugBox);
    }
}