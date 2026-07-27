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