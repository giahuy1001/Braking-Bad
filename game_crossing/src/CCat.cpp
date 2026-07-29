#include "CCat.h"
#include <iostream>

// Standard bounding box and moderate base speed
CCat::CCat(float startX, float startY, direction dir)
    : CAnimal(startX, startY, 40.0f, 40.0f, 200.0f, dir), burstTimer(0.0f), isBursting(false) {
}

void CCat::move(float dt) {
    burstTimer += dt;

    if (burstTimer > 1.5f) {
        isBursting = !isBursting;
        burstTimer = 0.0f;
    }

    // Apply BOTH the burst behavior AND the Endless mode scaling
    float currentSpeed = (isBursting ? (speed * 3.0f) : speed) * speedMultiplier;

    if (dir == RIGHT) {
        x += currentSpeed * dt;
    }
    else {
        x -= currentSpeed * dt;
    }
}