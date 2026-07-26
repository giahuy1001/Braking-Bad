#pragma once

#include <SFML/Graphics.hpp>
#include <string>

// Presentation-only character factory. Gameplay stores only the integer ID,
// so these placeholder shapes can later be replaced by sprites in one place.
class CharacterRenderer
{
public:
    static constexpr int kCharacterCount = 6;

    static int normalizeID(int id);
    static std::string name(int id);
    static void draw(sf::RenderTarget& target, int id, sf::Vector2f center, float radius);
};
