#include "CharacterRenderer.h"

#include <cmath>

namespace
{
    constexpr float PI = 3.14159265358979323846f;
    sf::Color colorFor(int id)
    {
        switch (id) {
        case 1: return sf::Color::Blue;
        case 2: return sf::Color::Red;
        case 3: return sf::Color::Yellow;
        case 4: return sf::Color::Green;
        case 5: return sf::Color::Magenta;
        default:return sf::Color(255, 145, 0); // orange diamond
        }
    }
}

int CharacterRenderer::normalizeID(int id)
{
    return ((id - 1) % kCharacterCount + kCharacterCount) % kCharacterCount + 1;
}

std::string CharacterRenderer::name(int id)
{
    const int normalized = normalizeID(id);
    return std::string("Character ") + (normalized < 10 ? "0" : "") + std::to_string(normalized);
}

void CharacterRenderer::draw(sf::RenderTarget& target, int id, sf::Vector2f center, float radius)
{
    id = normalizeID(id);
    const sf::Color color = colorFor(id);
    if (id == 1) {
        sf::RectangleShape shape({radius * 2.f, radius * 2.f});
        shape.setOrigin({radius, radius});
        shape.setPosition(center);
        shape.setFillColor(color);
        target.draw(shape);
        return;
    }
    if (id == 6) {
        sf::ConvexShape shape;
        shape.setPointCount(4);
        shape.setPoint(0, {0.f, -radius});
        shape.setPoint(1, {radius, 0.f});
        shape.setPoint(2, {0.f, radius});
        shape.setPoint(3, {-radius, 0.f});
        shape.setPosition(center);
        shape.setFillColor(color);
        target.draw(shape);
        return;
    }

    const std::size_t points = static_cast<std::size_t>(id == 2 ? 32 : id);
    sf::CircleShape shape(radius, points);
    shape.setOrigin({radius, radius});
    shape.setPosition(center);
    shape.setFillColor(color);
    target.draw(shape);
}
