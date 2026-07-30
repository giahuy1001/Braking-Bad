/**
 * @file Button.h
 * @brief Reusable interactive button control used by the game user interface.
 */
#pragma once

#include <SFML/Graphics.hpp>
#include "UIState.h"
#include <string>
#include <functional>

/**
 * @brief Represents one mouse- and keyboard-accessible UI control.
 *
 * The control stores transient hover and activation latches; navigation and
 * screen transitions remain the responsibility of UIManager.
 */
class Button
{
public:
    /** @brief Visual treatment applied to a button. */
    enum class Style { Primary, Subtle, Danger, IconOnly };

    /** @brief Constructs a button.
     * @param font Font used to measure the initial label.
     * @param pos Logical UI position.
     * @param size Logical UI size.
     * @param label Visible button text.
     * @param style Visual treatment.
     */
    Button(const sf::Font& font,
           sf::Vector2f pos, sf::Vector2f size, std::string label,
           Style style = Style::Primary);

    /** @return The button position. */
    sf::Vector2f position() const { return box_.getPosition(); }
    /** @return The button dimensions. */
    sf::Vector2f size()     const { return box_.getSize(); }
    /** @return The button hitbox. */
    sf::FloatRect bounds()  const { return box_.getGlobalBounds(); }

    /** @param e Enables or disables interaction. */
    void setEnabled(bool e);
    /** @return True when interaction is enabled. */
    bool isEnabled() const { return enabled_; }
    /** @param f Selects keyboard focus. */
    void setFocused(bool f);
    /** @param s Replacement visible label. */
    void setLabel(const std::string& s);
    /** @param a Opacity in the inclusive range [0, 1]. */
    void setAlpha(float a);

    /** @param mousePos Pointer position in UI coordinates. */
    void update(sf::Vector2f mousePos);
    /** @param mousePos Pointer position in UI coordinates. @return True when the hitbox contains it. */
    bool contains(sf::Vector2f mousePos) const;
    /** @param mousePos Pointer position in UI coordinates. @return True once for an armed click. */
    bool consumeClick(sf::Vector2f mousePos);
    /** @return True once for an armed Enter activation. */
    bool consumeEnter();

    /** @param win Destination render window. @param font Font attached before rendering. */
    void draw(sf::RenderWindow& win, const sf::Font& font);

private:
    sf::RectangleShape box_;
    sf::Text           label_;
    Style              style_;
    bool               enabled_     = true;
    bool               hovered_     = false;
    bool               focused_     = false;
    bool               clickArmed_  = false;
    bool               enterArmed_  = false;
    float              alpha_       = 1.0f;
    bool               fontAttached_= false;

    /** @brief Applies colors derived from interaction state and style. */
    void applyColors();
};
