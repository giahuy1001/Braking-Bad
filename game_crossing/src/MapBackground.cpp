/**
 * @file MapBackground.cpp
 * @brief Implements UI rendering and interaction behavior.
 */
#include "MapBackground.h"
#include "Grid.h"
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace fs = std::filesystem;
namespace {
std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}
}

void MapBackground::clear() { assets_.clear(); order_.clear(); }

/**
 * @brief Performs the load theme operation while preserving the current UI state invariants.
 */
bool MapBackground::loadTheme(const std::string& season)
{
    clear();
    const std::string titled = season.empty() ? season : std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(season[0])))) + season.substr(1);
    const std::vector<fs::path> candidates = { fs::path("assets/theme") / season / "map", fs::path("assets/theme") / titled / "map" };
    fs::path directory;
    for (const fs::path& candidate : candidates) if (fs::is_directory(candidate)) { directory = candidate; break; }
    if (directory.empty()) return false;
    for (const fs::directory_entry& entry : fs::directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;
        const std::string ext = lower(entry.path().extension().string());
        if (ext != ".png" && ext != ".jpg" && ext != ".jpeg") continue;
        auto asset = std::make_unique<Asset>();
        if (!asset->texture.loadFromFile(entry.path().string())) continue;
        asset->sprite.setTexture(asset->texture, true);
        const std::string key = entry.path().stem().string();
        assets_.emplace(key, std::move(asset));
        order_.push_back(key);
    }
    std::sort(order_.begin(), order_.end());
    return !order_.empty();
}

/**
 * @brief Performs the available level numbers operation while preserving the current UI state invariants.
 */
std::vector<int> MapBackground::availableLevelNumbers() const
{
    constexpr const char* prefix = "map_level_";
    std::vector<int> levels;
    for (const std::string& key : order_) {
        if (key.rfind(prefix, 0) != 0) continue;
        const std::string suffix = key.substr(std::char_traits<char>::length(prefix));
        if (suffix.empty() || !std::all_of(suffix.begin(), suffix.end(), [](unsigned char c) { return std::isdigit(c) != 0; }))
            continue;
        levels.push_back(std::stoi(suffix));
    }
    return levels;
}

/**
 * @brief Performs the resolve operation while preserving the current UI state invariants.
 */
const MapBackground::Asset* MapBackground::resolve(const std::string& key, std::uint32_t fallbackIndex) const
{
    if (const auto it = assets_.find(key); it != assets_.end()) return it->second.get();
    if (order_.empty()) return nullptr;
    return assets_.at(order_[fallbackIndex % order_.size()]).get();
}

/**
 * @brief Performs the draw block operation while preserving the current UI state invariants.
 */
void MapBackground::drawBlock(sf::RenderTarget& target, const std::string& key, std::uint32_t fallbackIndex, float worldStartY, float blockHeight, float cameraY) const
{
    const Asset* asset = resolve(key, fallbackIndex);
    if (!asset) return;
    const float screenY = worldStartY - cameraY;
    if (screenY >= Grid::MAP_HEIGHT || screenY + blockHeight <= 0.f) return;
    sf::Sprite sprite = asset->sprite;
    const sf::Vector2u size = asset->texture.getSize();
    if (size.x == 0 || size.y == 0) return;

    sprite.setPosition({ 0.f, screenY });
    target.draw(sprite);
}
