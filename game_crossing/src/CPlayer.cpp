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

void CPlayer::resetPosition() {
    x = spawnPosition_.x; y = spawnPosition_.y;
    moving_ = false; hasShield_ = false; invincibleTimer_ = 0.f;
    revive();
}

void CPlayer::setSkin(int skinID)
{
    skinID_ = CharacterRenderer::normalizeID(skinID);
}

void CPlayer::revive() {
    alive_ = true; moving_ = false; hasShield_ = false; invincibleTimer_ = 0.f;
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

// Restored to correctly offset the AABB from the center point to the top-left
sf::FloatRect CPlayer::getBounds() const
{
    return sf::FloatRect({ x - radius_, y - radius_ }, { radius_ * 2.f, radius_ * 2.f });
}

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

void CPlayer::update(float dt) {
    if (invincibleTimer_ > 0.f) invincibleTimer_ -= dt;
    moving_ = false;
}

// 3. RENDER RENAMED TO DRAW: To properly override CGameObject's virtual function
void CPlayer::draw(sf::RenderWindow& window, float cameraY) {
    if (invincibleTimer_ > 0.f) {
        // Nhấp nháy (tàng hình đứt quãng mỗi 0.1s)
        if (static_cast<int>(invincibleTimer_ * 15.f) % 2 == 0) return;
    }
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

void CPlayer::giveShield() { hasShield_ = true; }
bool CPlayer::hasShield() const { return hasShield_; }
void CPlayer::consumeShield() { hasShield_ = false; invincibleTimer_ = 1.0f; }
bool CPlayer::isInvincible() const { return invincibleTimer_ > 0.f; }