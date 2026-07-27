#pragma once
#include "CGameObstacle.h"

class CVehicle : public CObstacle {
private:
    bool isStopping;
public:
    CVehicle(float startX, float startY, float w, float h, float speed, direction dir);
    bool getStoppingStatus();
    void stop();
    void continueMoving();
    virtual void move(float dt) override = 0;
};