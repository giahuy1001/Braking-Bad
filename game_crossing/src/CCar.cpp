#include "CCar.h"

CCar::CCar(float startX, float startY, direction dir)
    : CVehicle(startX, startY, 80.0f, 50.0f, 400.0f, dir) {
}

void CCar::move(float dt) {
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