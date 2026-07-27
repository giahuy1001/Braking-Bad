#pragma once
#include "CAnimal.h"

class CCat : public CAnimal {
private:
    float burstTimer;
    bool isBursting;

public:
    CCat(float startX, float startY, direction dir);
    void move(float dt) override;
    void tell() override;
};