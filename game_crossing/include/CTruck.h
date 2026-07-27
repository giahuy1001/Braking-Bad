#pragma once
#include "CVehicle.h"

class CTruck : public CVehicle {
public:
    CTruck(float startX, float startY, direction dir);
    void move(float dt) override;
};