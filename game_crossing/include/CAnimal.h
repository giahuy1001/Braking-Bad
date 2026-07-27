#pragma once
#include "CGameObstacle.h"

class CAnimal : public CObstacle {
public:
    CAnimal(float startX, float startY, direction dir);
    void move() override;
    void tell();
};