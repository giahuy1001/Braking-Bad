#include "CPlayer.h"
#include "CharacterRenderer.h"
#include "Grid.h"
#include <algorithm>

// 1. CONSTRUCTOR UPDATE: Pass coordinates and size to CGameObject
CPlayer::CPlayer(sf::Vector2f spawnPosition, int skinID, float radius)
    : CGameObject(spawnPosition.x, spawnPosition.y, radius * 2.f, radius * 2.f),
    spawnPosition_(spawnPosition), radius_(radius)
{
    setSkin(skinID);
}

void CPlayer::setSpawnPosition(sf::Vector2f pos)
{
    spawnPosition_ = pos;
}

void CPlayer::resetPosition()
{
    // 2. USE BASE CLASS COORDINATES
    x = spawnPosition_.x;
    y = spawnPosition_.y;
    moving_ = false;
    revive();
}

void CPlayer::setSkin(int skinID)
{
    skinID_ = CharacterRenderer::normalizeID(skinID);
}

void CPlayer::revive()
{
    alive_ = true;
    moving_ = false;
}

void CPlayer::kill()
{
    alive_ = false;
    moving_ = false;
}

bool CPlayer::isAlive() const { return alive_; }
bool CPlayer::isMoving() const { return moving_; }

// Pack x and y back into an sf::Vector2f for any legacy code expecting it
sf::Vector2f CPlayer::getPosition() const { return { x, y }; }

void CPlayer::setPosition(sf::Vector2f pos)
{
    // Loading a save deliberately bypasses current viewport movement bounds.
    x = pos.x;
    y = pos.y;
    moving_ = false;
}

// (getBounds() was deleted here since it is now inherited from CGameObject)

void CPlayer::setMovementBounds(sf::FloatRect bounds)
{
    movementBounds_ = bounds;
}

void CPlayer::setCameraOffset(float cameraY)
{
    cameraY_ = cameraY;
}

void CPlayer::handleInput()
{
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
        moveByGridStep(-Grid::CELL_SIZE, 0.f); return;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
        moveByGridStep(Grid::CELL_SIZE, 0.f); return;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
        moveByGridStep(0.f, -Grid::CELL_SIZE); return;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) moveByGridStep(0.f, Grid::CELL_SIZE);
}

void CPlayer::handleInput(sf::Keyboard::Key key)
{
    switch (key) {
    case sf::Keyboard::Key::Left:
    case sf::Keyboard::Key::A: moveByGridStep(-Grid::CELL_SIZE, 0.f); break;
    case sf::Keyboard::Key::Right:
    case sf::Keyboard::Key::D: moveByGridStep(Grid::CELL_SIZE, 0.f); break;
    case sf::Keyboard::Key::Up:
    case sf::Keyboard::Key::W: moveByGridStep(0.f, -Grid::CELL_SIZE); break;
    case sf::Keyboard::Key::Down:
    case sf::Keyboard::Key::S: moveByGridStep(0.f, Grid::CELL_SIZE); break;
    default: break;
    }
}

void CPlayer::update(float /*dt*/)
{
    // Movement is intentionally committed on each KeyPressed event, matching
    // the existing game. No interpolation is introduced by this refactor.
    moving_ = false;
}

// 3. RENDER RENAMED TO DRAW: To properly override CGameObject's virtual function
void CPlayer::draw(sf::RenderWindow& window, float cameraY)
{
    CharacterRenderer::draw(window, skinID_, { x, y - cameraY }, radius_);
}

void CPlayer::moveByGridStep(float dx, float dy)
{
    if (!alive_) return;

    // Use base class x and y
    float nextX = x + dx;
    float nextY = y + dy;

    // Horizontal movement is clamped; an invalid vertical step is rejected.
    // This exactly prevents the old off-screen snap at the bottom boundary.
    nextX = std::clamp(nextX, movementBounds_.position.x,
        movementBounds_.position.x + movementBounds_.size.x);
    if (dy == 0.f || (nextY >= movementBounds_.position.y &&
        nextY <= movementBounds_.position.y + movementBounds_.size.y))
        y = nextY;
    x = nextX;
    moving_ = true;
}

// 4. COLLISION LOGIC: Implemented using inherited getBounds()
bool CPlayer::isImpact(CVehicle* vehicle) {
    if (vehicle == nullptr) return false;

    // SFML 3.0.2 AABB intersection check
    return this->getBounds().findIntersection(vehicle->getBounds()).has_value();
}

bool CPlayer::isImpact(CAnimal* animal) {
    if (animal == nullptr) return false;

    // SFML 3.0.2 AABB intersection check
    return this->getBounds().findIntersection(animal->getBounds()).has_value();
}