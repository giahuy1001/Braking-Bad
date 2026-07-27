#include "../include/CAnimal.h"

// call the constructor of CObstacle to initialize position, size, and speed
// Pass the parameters directly up to CObstacle
CAnimal::CAnimal(float startX, float startY, float w, float h, float speed, direction dir)
    : CObstacle(startX, startY, w, h, speed, dir) {
}
