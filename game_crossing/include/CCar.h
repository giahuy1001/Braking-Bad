#pragma once
#include "CVehicle.h"

class CCar : public CVehicle {
public:
    CCar(float startX, float startY, direction dir);
    void move(float dt) override;
};
