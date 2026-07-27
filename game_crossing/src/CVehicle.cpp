#include "../include/CGameObstacle.h"
#include "../include/CVehicle.h"

CVehicle::CVehicle(float startX, float startY, direction dir)
    : CObstacle(startX, startY, 120.0f, 60.0f, 8.0f, dir) {
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

void CVehicle::move() 
{
    if (dir == RIGHT)
    {
        x += speed;
        return;
    }
        x -= speed;
}