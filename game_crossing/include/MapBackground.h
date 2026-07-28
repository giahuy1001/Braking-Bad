#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Owns the gameplay-map artwork for the active season. Map geometry remains
// in MapBlock; this class only turns its world-space rectangle into a sprite.
class MapBackground
{
public:
    bool loadTheme(const std::string& season);
    void clear();
    // Numeric map levels with a matching image in the active theme.
    std::vector<int> availableLevelNumbers() const;
    void drawBlock(sf::RenderTarget& target, const std::string& key,
                   std::uint32_t fallbackIndex, float worldStartY,
                   float blockHeight, float cameraY) const;

private:
    struct Asset {
        sf::Texture texture;
        sf::Sprite sprite;

        // SFML 3 has no default constructor for Sprite: it must always be
        // associated with a texture at construction time.
        Asset() : sprite(texture) {}
    };
    const Asset* resolve(const std::string& key, std::uint32_t fallbackIndex) const;
    std::unordered_map<std::string, std::unique_ptr<Asset>> assets_;
    std::vector<std::string> order_;
};
