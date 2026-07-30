#include "CGameObstacle.h"

// Returns true if the obstacle has completely left the playable grid
bool CObstacle::isOffScreen() const {
    // If moving left, check if the right edge of the sprite (x + width) is past 0
    if (dir == LEFT && (x + width) < Grid::GRID_LEFT) {
        return true;
    }
    // If moving right, check if the left edge (x) is past the grid's maximum width
    if (dir == RIGHT && x > Grid::GRID_RIGHT) {
        return true;
    }
    return false;
}

void CObstacle::updateAnimation(float dt) {
    if (!sprite || totalFrames <= 1) return;

    animTimer += dt;
    if (animTimer >= frameDuration) {
        animTimer -= frameDuration;
        currentFrame = (currentFrame + 1) % totalFrames;

        int col = currentFrame % frameCols;
        int row = currentFrame / frameCols;

        // Cú pháp SFML 3: sf::IntRect({x, y}, {width, height})
        sprite->setTextureRect(sf::IntRect(
            { col * frameWidth, row * frameHeight },
            { frameWidth, frameHeight }
        ));
    }
}