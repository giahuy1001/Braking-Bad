#pragma once
#include "CGameObstacle.h"

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