#pragma once

#include <SFML/Graphics.hpp>
#include <string>

// Quản lý và vẽ Sprite nhân vật từ sprite sheet (3 cột x 4 hàng)
class CharacterRenderer
{
public:
    static constexpr int kCharacterCount = 4; // Cập nhật thành 4 nhân vật

    static void loadAssets();
    static int normalizeID(int id);
    static std::string name(int id);

    // Thêm directionRow (0: Xuống, 1: Trái, 2: Phải, 3: Lên) và frameCol (0, 1, 2)
    static void draw(sf::RenderTarget& target, int id, sf::Vector2f center, float radius, int directionRow = 0, int frameCol = 1);
};