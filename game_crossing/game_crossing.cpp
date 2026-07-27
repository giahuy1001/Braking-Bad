#include <SFML/Graphics.hpp>
#include "UIManager.h"

int main()
{
    sf::RenderWindow window(
        sf::VideoMode({ 1920, 1080 }),
        "Zombie Apocalypse",
        sf::Style::None
    );

    window.setPosition({ 0, 0 });
    window.setFramerateLimit(60);

    UIManager ui(window);
    ui.run();

    return 0;
}