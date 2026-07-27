#pragma once
#include "CGameObstacle.h"

float const defaultVehicleSpeed = 480.0f;

class CVehicle : public CObstacle {
private:
    bool isStopping;
public:
    CVehicle(float startX, float startY, direction dir);
    bool getStatus();
    void stop();
    void continueMoving();
    void move(float dt) override;
};