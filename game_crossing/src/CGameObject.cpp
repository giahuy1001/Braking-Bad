#include "CGameObject.h"

CGameObject::CGameObject(float startX, float startY, float w, float h) {
    x = startX;
    y = startY;
    width = w;
    height = h;
    sprite = nullptr;
}

CGameObject::~CGameObject() {
    if (sprite != nullptr) {
        delete sprite;
        sprite = nullptr;
    }
}

void CGameObject::draw(sf::RenderWindow& window) {
    if (sprite != nullptr) {
        sprite->setPosition({ x, y });

        window.draw(*sprite);
    }
}