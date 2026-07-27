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