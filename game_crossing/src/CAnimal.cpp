#include "../include/CAnimal.h"

// call the constructor of CObstacle to initialize position, size, and speed
CAnimal::CAnimal(float startX, float startY, direction dir)
    : CObstacle(startX, startY, 40.0f, 60.0f, 4.0f, dir) {
}

// move function
void CAnimal::move() {
    if (dir == RIGHT)
    {
        x += speed;
        return;
    }
    x -= speed;
}

void CAnimal::tell()
{

}