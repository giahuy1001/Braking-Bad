/**
 * @file Button.cpp
 * @brief Implements UI rendering and interaction behavior.
 */
#include "Button.h"
#include <cmath>

namespace
{
    constexpr float UI_SCALE = 1.5f;

    sf::Vector2f toLogicalUi(sf::Vector2f pixelPosition)
    {
        return { pixelPosition.x / UI_SCALE, pixelPosition.y / UI_SCALE };
    }

    sf::Color fillFor(Button::Style s, bool enabled, bool hover, bool focus)
    {
        if (!enabled) return sf::Color(70, 70, 70, 180);
        switch (s)
        {
        case Button::Style::Primary:
            return (hover || focus) ? sf::Color(70, 110, 170, 230)
                                    : sf::Color(50,  80, 130, 210);
        case Button::Style::Subtle:
            return (hover || focus) ? sf::Color(80,  80,  80, 200)
                                    : sf::Color(50,  50,  50, 170);
        case Button::Style::Danger:
            return (hover || focus) ? sf::Color(180,  60,  60, 230)
                                    : sf::Color(140,  40,  40, 210);
        case Button::Style::IconOnly:
            return (hover || focus) ? sf::Color(255, 255, 255, 70)
                                    : sf::Color(255, 255, 255, 30);
        }
        return sf::Color::White;
    }

    sf::Color borderFor(Button::Style s)
    {
        switch (s)
        {
        case Button::Style::Danger:  return sf::Color(220, 80, 80);
        case Button::Style::Subtle:  return sf::Color(140, 140, 140);
        case Button::Style::IconOnly:return sf::Color(255, 255, 255, 120);
        default:                     return sf::Color(180, 200, 240);
        }
    }
}

Button::Button(const sf::Font& font,
                 sf::Vector2f pos, sf::Vector2f size, std::string label, Style style)
    : box_(size), label_(font, std::move(label),
                         style == Style::IconOnly ? 22u : 24u),
      style_(style), fontAttached_(true)
{
    box_.setPosition(pos);
    box_.setOutlineThickness(-2.f);
    applyColors();
}

/**
 * @brief Performs the set enabled operation while preserving the current UI state invariants.
 */
void Button::setEnabled(bool e)
{
    enabled_ = e;
    applyColors();
}

/**
 * @brief Performs the set focused operation while preserving the current UI state invariants.
 */
void Button::setFocused(bool f)
{
    focused_ = f;
    applyColors();
}

/**
 * @brief Performs the set label operation while preserving the current UI state invariants.
 */
void Button::setLabel(const std::string& s)
{
    label_.setString(s);
}

/**
 * @brief Performs the set alpha operation while preserving the current UI state invariants.
 */
void Button::setAlpha(float a)
{
    alpha_ = std::clamp(a, 0.f, 1.f);
}

/**
 * @brief Advances state that depends on elapsed time after input has established the current intent.
 */
void Button::update(sf::Vector2f mousePos)
{
    if (!enabled_) { hovered_ = false; return; }
    hovered_ = contains(mousePos);
}

/**
 * @brief Performs the contains operation while preserving the current UI state invariants.
 */
bool Button::contains(sf::Vector2f mousePos) const
{
    return enabled_ && bounds().contains(toLogicalUi(mousePos));
}

/**
 * @brief Performs the consume click operation while preserving the current UI state invariants.
 */
bool Button::consumeClick(sf::Vector2f mousePos)
{
    if (!enabled_) return false;

    return bounds().contains(toLogicalUi(mousePos));
}

/**
 * @brief Performs the consume enter operation while preserving the current UI state invariants.
 */
bool Button::consumeEnter()
{
    if (!enabled_ || !focused_) { enterArmed_ = false; return false; }
    if (!enterArmed_) { enterArmed_ = true; return false; }
    enterArmed_ = false;
    return true;
}

/**
 * @brief Performs the apply colors operation while preserving the current UI state invariants.
 */
void Button::applyColors()
{
    box_.setFillColor   (fillFor(style_, enabled_, hovered_, focused_));
    box_.setOutlineColor(borderFor(style_));
    label_.setFillColor(enabled_ ? sf::Color::White : sf::Color(150, 150, 150, 200));
}

/**
 * @brief Performs the draw operation while preserving the current UI state invariants.
 */
void Button::draw(sf::RenderWindow& win, const sf::Font& font)
{
    if (!fontAttached_)
    {
        label_.setFont(font);
        fontAttached_ = true;
    }

    const sf::FloatRect lb = label_.getLocalBounds();
    label_.setOrigin({ lb.position.x + lb.size.x * 0.5f,
                       lb.position.y + lb.size.y * 0.5f });
    label_.setPosition({ box_.getPosition().x + box_.getSize().x * 0.5f,
                         box_.getPosition().y + box_.getSize().y * 0.55f });

    const float a = alpha_;
    const sf::Color fc = box_.getFillColor();
    const sf::Color oc = box_.getOutlineColor();
    box_.setFillColor   ({ fc.r, fc.g, fc.b, static_cast<std::uint8_t>(fc.a * a) });
    box_.setOutlineColor({ oc.r, oc.g, oc.b, static_cast<std::uint8_t>(oc.a * a) });

    const sf::Color lc = label_.getFillColor();
    label_.setFillColor({ lc.r, lc.g, lc.b, static_cast<std::uint8_t>(lc.a * a) });

    win.draw(box_);
    win.draw(label_);

    box_.setFillColor(fc);
    box_.setOutlineColor(oc);
    label_.setFillColor(lc);
}
