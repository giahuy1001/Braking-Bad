#pragma once

#include <SFML/Graphics.hpp>
#include "UIState.h"
#include <string>
#include <functional>

// ---------------------------------------------------------------------
//  Reusable rectangular button widget.
//
//  Supports both mouse (hover + click) and keyboard (focus + Enter).
//  The "small icon on the right side of the screen" described in the
//  spec is just a Button with size = {40, 40} and an arrow glyph.
//
//  Action on activation: invokes callback_.  We do NOT encode the target
//  UIState here because the same button (e.g. "Back") needs different
//  transitions depending on which screen we are on — the UIManager owns
//  that mapping.
// ---------------------------------------------------------------------
class Button
{
public:
    enum class Style { Primary, Subtle, Danger, IconOnly };

    Button(const sf::Font& font,
           sf::Vector2f pos, sf::Vector2f size, std::string label,
           Style style = Style::Primary);

    // Geometry helpers
    sf::Vector2f position() const { return box_.getPosition(); }
    sf::Vector2f size()     const { return box_.getSize(); }
    sf::FloatRect bounds()  const { return box_.getGlobalBounds(); }

    // State
    void setEnabled(bool e);
    bool isEnabled() const { return enabled_; }
    void setFocused(bool f);
    void setLabel(const std::string& s);
    void setAlpha(float a);                 // 0..1, used for Back icon fade

    // Event helpers
    void update(sf::Vector2f mousePos);
    bool contains(sf::Vector2f mousePos) const;
    bool consumeClick(sf::Vector2f mousePos);   // returns true exactly once per press
    bool consumeEnter();                        // returns true exactly once per Enter

    // Draw
    void draw(sf::RenderWindow& win, const sf::Font& font);

private:
    sf::RectangleShape box_;
    sf::Text           label_;
    Style              style_;
    bool               enabled_     = true;
    bool               hovered_     = false;
    bool               focused_     = false;
    bool               clickArmed_  = false;     // latches between press/release
    bool               enterArmed_  = false;
    float              alpha_       = 1.0f;
    bool               fontAttached_= false;     // sf::Text has no default ctor;
                                                // we attach the font lazily in draw()

    void applyColors();
};