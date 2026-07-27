#include "../include/CGameObstacle.h"
#include "../include/CVehicle.h"

// Pass the parameters directly up to CObstacle
CVehicle::CVehicle(float startX, float startY, float w, float h, float speed, direction dir)
    : CObstacle(startX, startY, w, h, speed, dir), isStopping(false) {
}

bool CVehicle::getStoppingStatus()
{
    if (isStopping == true)
        return true;

    return false;
}

void CVehicle::stop()
{
    isStopping = true;
}

void CVehicle::continueMoving()
{
    isStopping = false;
}