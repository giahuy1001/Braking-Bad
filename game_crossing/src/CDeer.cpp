#include "CDeer.h"
#include <iostream>
#include <cmath>

// Larger bounding box for the deer and a slightly higher base speed
CDeer::CDeer(float startX, float startY, direction dir)
    : CAnimal(startX, startY, 60.0f, 60.0f, 250.0f, dir), leapCycle(0.0f) {
}

void CDeer::move(float dt) {
    leapCycle += dt * 4.0f;

    float leapMultiplier = std::abs(std::sin(leapCycle)) * 2.5f;

    // Apply BOTH the leap math AND the Endless mode scaling
    float currentSpeed = (speed + (speed * leapMultiplier)) * speedMultiplier;

    if (dir == RIGHT) {
        x += currentSpeed * dt;
    }
    else {
        x -= currentSpeed * dt;
    }
}

void CDeer::tell() {
    // Mimics the sound they make in real life
    std::cout << "Bleat!" << std::endl;
}