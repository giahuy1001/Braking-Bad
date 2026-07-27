#pragma once
#include "CAnimal.h"

class CDeer : public CAnimal {
private:
    float leapCycle;

public:
    CDeer(float startX, float startY, direction dir);
    void move(float dt) override;
    void tell() override;
};