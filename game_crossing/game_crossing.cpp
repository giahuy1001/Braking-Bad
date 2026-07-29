#include <SFML/Graphics.hpp>
#include "UIManager.h"
#include <SFML/Window/Context.hpp>

int main()
{
    // Keeps the GPU memory alive when the window is destroyed/recreated
    sf::Context dummyContext;

    sf::RenderWindow window(
        sf::VideoMode({ 1920, 1080 }), 
        "Braking Bad", 
        sf::Style::Default
    );

    window.setPosition({ 0, 0 });
    window.setFramerateLimit(60);

    UIManager ui(window);
    ui.run();

    return 0;
}