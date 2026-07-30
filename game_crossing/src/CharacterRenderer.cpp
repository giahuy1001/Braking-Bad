/**
 * @file CharacterRenderer.cpp
 * @brief Implements UI rendering and interaction behavior.
 */
#include "CharacterRenderer.h"
#include <iostream>

namespace {
    sf::Texture textures[CharacterRenderer::kCharacterCount];
    bool isLoaded = false;
}

void CharacterRenderer::loadAssets() {
    if (isLoaded) return;
    for (int i = 0; i < kCharacterCount; ++i) {

        std::string path = "assets/characters/character" + std::to_string(i + 1) + ".png";
        if (!textures[i].loadFromFile(path)) {
            std::cerr << "[CharacterRenderer] Warning: Could not load " << path << "\n";
        }
    }
    isLoaded = true;
}

int CharacterRenderer::normalizeID(int id) {
    return ((id - 1) % kCharacterCount + kCharacterCount) % kCharacterCount + 1;
}

std::string CharacterRenderer::name(int id) {
    const int normalized = normalizeID(id);
    return std::string("Character ") + (normalized < 10 ? "0" : "") + std::to_string(normalized);
}

void CharacterRenderer::draw(sf::RenderTarget& target, int id, sf::Vector2f center, float radius, int directionRow, int frameCol) {
    loadAssets();

    id = normalizeID(id);
    sf::Texture& tex = textures[id - 1];

    if (tex.getSize().x == 0) {
        sf::CircleShape fallback(radius);
        fallback.setOrigin({ radius, radius });
        fallback.setPosition(center);
        fallback.setFillColor(sf::Color::Red);
        target.draw(fallback);
        return;
    }

    sf::Sprite sprite(tex);

    int frameWidth = tex.getSize().x / 3;
    int frameHeight = tex.getSize().y / 4;

    sprite.setTextureRect(sf::IntRect({ frameCol * frameWidth, directionRow * frameHeight }, { frameWidth, frameHeight }));

    sprite.setOrigin({ frameWidth / 2.0f, frameHeight / 2.0f });

    float scale = (radius * 2.2f) / frameHeight;
    sprite.setScale({ scale, scale });

    sprite.setPosition(center);
    target.draw(sprite);
}
