#include "../include/CGameObstacle.h"
#include "../include/CVehicle.h"

CVehicle::CVehicle(float startX, float startY, direction dir)
    : CObstacle(startX, startY, 120.0f, 60.0f, 8.0f, dir), isStopping(false) {
}

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
    // If the vehicle is supposed to be stopped, exit the function immediately!
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