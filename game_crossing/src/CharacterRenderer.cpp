#include "CharacterRenderer.h"
#include <iostream>

namespace {
    sf::Texture textures[CharacterRenderer::kCharacterCount];
    bool isLoaded = false;
}

void CharacterRenderer::loadAssets() {
    if (isLoaded) return;
    for (int i = 0; i < kCharacterCount; ++i) {
        // Nạp 4 file character1.png đến character4.png
        std::string path = "assets/characters/character" + std::to_string(i + 1) + ".png";
        if (!textures[i].loadFromFile(path)) {
            std::cerr << "[CharacterRenderer] Warning: Could not load " << path << "\n";
        }
    }
    isLoaded = true;
}

int CharacterRenderer::normalizeID(int id) {
    return ((id - 1) % kCharacterCount + kCharacterCount) % kCharacterCount + 1;
}

std::string CharacterRenderer::name(int id) {
    const int normalized = normalizeID(id);
    return std::string("Character ") + (normalized < 10 ? "0" : "") + std::to_string(normalized);
}

void CharacterRenderer::draw(sf::RenderTarget& target, int id, sf::Vector2f center, float radius, int directionRow, int frameCol) {
    loadAssets(); // Lazy load: Tự động nạp ảnh ở lần vẽ đầu tiên

    id = normalizeID(id);
    sf::Texture& tex = textures[id - 1];

    // Nếu chưa có file ảnh, vẽ một hình tròn đỏ làm fallback báo lỗi
    if (tex.getSize().x == 0) {
        sf::CircleShape fallback(radius);
        fallback.setOrigin({ radius, radius });
        fallback.setPosition(center);
        fallback.setFillColor(sf::Color::Red);
        target.draw(fallback);
        return;
    }

    sf::Sprite sprite(tex);

    // Tính toán kích thước 1 khung hình (Sheet có 3 cột, 4 hàng)
    int frameWidth = tex.getSize().x / 3;
    int frameHeight = tex.getSize().y / 4;

    // Cắt đúng vị trí frame theo hướng (hàng) và nhịp bước (cột)
    sprite.setTextureRect(sf::IntRect({ frameCol * frameWidth, directionRow * frameHeight }, { frameWidth, frameHeight }));

    // Chỉnh tâm bức ảnh về giữa
    sprite.setOrigin({ frameWidth / 2.0f, frameHeight / 2.0f });

    // Scale sprite lên cho vừa vặn với hitbox (dựa trên radius)
    float scale = (radius * 2.2f) / frameHeight;
    sprite.setScale({ scale, scale });

    sprite.setPosition(center);
    target.draw(sprite);
}