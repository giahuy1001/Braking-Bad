#pragma once
#include "CGameObject.h"
#include "Grid.h"

enum direction
{
	LEFT, RIGHT
};

class CObstacle : public CGameObject {
protected:
    float speed;
    float speedMultiplier = 1.0f;
    direction dir;

    int currentFrame = 0;
    float animTimer = 0.0f;
    float frameDuration = 0.1f; // Tốc độ lật frame
    int totalFrames = 1;
    int frameCols = 1;
    int frameWidth = 0;
    int frameHeight = 0;

    void updateAnimation(float dt);
public:
    // recall constructor to reinitialize position, size, and speed
    CObstacle(float startX, float startY, float w, float h, float spd, direction dir)
        : CGameObject(startX, startY, w, h), speed{ spd }, dir{ dir } {
    }

    // virtual move function
    virtual void move(float dt) = 0;

    direction getDirection() {
        return dir;
	}

    float getX() {
        return x;
	}

    float getY() {
        return y;
    }

    void setSpeedMultiplier(float mult) {
        speedMultiplier = mult;
    }

    bool isOffScreen() const;
};
