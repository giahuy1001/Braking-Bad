#pragma once
#include "CGameObject.h"

class CObstacle : public CGameObject {
protected:
    float speed;

public:
    CObstacle(float startX, float startY, float w, float h, float spd)
        : CGameObject(startX, startY, w, h), speed(spd) {
    }

    virtual void move() = 0;

    virtual bool freeze() {
        return false;
    }
};