/**
 * @file CharacterRenderer.h
 * @brief Character preview renderer used by the graphics-selection interface.
 */
#pragma once

#include <SFML/Graphics.hpp>
#include <string>

/** @brief Draws normalized character previews for the graphics-selection screen. */
class CharacterRenderer
{
public:
    /** @brief Number of selectable character identities. */
    static constexpr int kCharacterCount = 4;

    /** @brief Loads preview assets when required by the renderer. */
    static void loadAssets();
    /** @param id Candidate identifier. @return A valid character identifier. */
    static int normalizeID(int id);
    /** @param id Character identifier. @return Display name for that character. */
    static std::string name(int id);

    /** @brief Draws one character preview.
     * @param target Destination render target. @param id Character identifier.
     * @param center Preview center. @param radius Requested preview radius.
     * @param directionRow Sprite-sheet direction row. @param frameCol Sprite-sheet animation column.
     */
    static void draw(sf::RenderTarget& target, int id, sf::Vector2f center, float radius, int directionRow = 0, int frameCol = 1);
};
