#include "../include/CGameObstacle.h"
#include "../include/CVehicle.h"

CVehicle::CVehicle(float startX, float startY, direction dir)
    : CObstacle(startX, startY, 120.0f, 60.0f, defaultVehicleSpeed, dir), isStopping(false) //x, y, width, height, speed, dir
{}

bool CVehicle::getStatus()
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

void CVehicle::move(float dt)
{
    if (isStopping) {
        return;
    }

    if (dir == RIGHT)
    {
        x += speed * dt;
        return;
    }
    x -= speed * dt;
}