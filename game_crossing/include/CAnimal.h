#pragma once
#include "CGameObstacle.h"

float const defaultAnimalSpeed = 240.0f;

class CAnimal : public CObstacle {
public:
    CAnimal(float startX, float startY, float w, float h, float speed, direction dir);
    virtual void move(float dt) override = 0;
};