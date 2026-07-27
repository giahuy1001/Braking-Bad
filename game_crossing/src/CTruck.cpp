#include "CTruck.h"

// Implements a larger bounding box (160x60) and slower velocity (6.0f)
CTruck::CTruck(float startX, float startY, direction dir)
    : CVehicle(startX, startY, 160.0f, 60.0f, 300.0f, dir) {
}

void CTruck::move(float dt) {
    if (getStoppingStatus()) {
        return;
    }

    // Factor in the Endless Mode multiplier
    float currentSpeed = speed * speedMultiplier;

    if (dir == RIGHT) {
        x += currentSpeed * dt;
    }
    else {
        x -= currentSpeed * dt;
    }
}