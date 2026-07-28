#pragma once
#include <SFML/Graphics.hpp>

class CGameObject {
protected:
    float x;
    float y;
    float width;
    float height;

    sf::Sprite* sprite;

public:
    //constructor
    CGameObject(float startX, float startY, float w, float h);

    //destructor
    virtual ~CGameObject();

    virtual void draw(sf::RenderWindow& window, float cameraY);
    sf::FloatRect getBounds() const {
        return sf::FloatRect({ x, y }, { width, height });
    }
};