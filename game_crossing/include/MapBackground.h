/**
 * @file MapBackground.h
 * @brief Season-aware renderer for gameplay map artwork.
 */
#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/** @brief Owns map artwork for one seasonal theme and renders world-space blocks. */
class MapBackground
{
public:
    /** @param season Theme directory name. @return True when usable map artwork was loaded. */
    bool loadTheme(const std::string& season);
    /** @brief Releases all loaded seasonal map artwork. */
    void clear();

    /** @return Sorted level numbers available in the active theme. */
    std::vector<int> availableLevelNumbers() const;
    /** @brief Draws one world-space map block.
     * @param target Destination render target. @param key Preferred artwork key.
     * @param fallbackIndex Deterministic fallback artwork index. @param worldStartY Block world origin.
     * @param blockHeight Block height in world units. @param cameraY Current camera offset.
     */
    void drawBlock(sf::RenderTarget& target, const std::string& key,
                   std::uint32_t fallbackIndex, float worldStartY,
                   float blockHeight, float cameraY) const;

private:
    /** @brief Texture and sprite kept together because SFML sprites reference their texture. */
    struct Asset {
        sf::Texture texture;
        sf::Sprite sprite;

        /** @brief Constructs a sprite already bound to its owned texture. */
        Asset() : sprite(texture) {}
    };
    /** @brief Resolves preferred artwork, then a deterministic fallback.
     * @return The resolved asset, or null when no artwork is loaded.
     */
    const Asset* resolve(const std::string& key, std::uint32_t fallbackIndex) const;
    std::unordered_map<std::string, std::unique_ptr<Asset>> assets_;
    std::vector<std::string> order_;
};
