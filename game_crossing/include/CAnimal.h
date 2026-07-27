#pragma once
#include "CGameObstacle.h"

float const defaultAnimalSpeed = 240.0f;

class CAnimal : public CObstacle {
public:
    CAnimal(float startX, float startY, direction dir);
    void tell();
    void move(float dt) override;
};