#include "CPlayer.h"
#include "CharacterRenderer.h"
#include "Grid.h"
#include <algorithm>

CPlayer::CPlayer(sf::Vector2f spawnPosition, int skinID, float radius)
    : CGameObject(spawnPosition.x, spawnPosition.y, radius * 2.f, radius * 2.f),
    spawnPosition_(spawnPosition), radius_(radius)
{
    setSkin(skinID);
}

void CPlayer::setSpawnPosition(sf::Vector2f pos) { spawnPosition_ = pos; }

void CPlayer::resetPosition() {
    x = spawnPosition_.x; y = spawnPosition_.y;
    moving_ = false; hasShield_ = false; invincibleTimer_ = 0.f;
    facingDir_ = 0; animFrame_ = 1; // Nhìn xuống và đứng im
    revive();
}

void CPlayer::setSkin(int skinID) { skinID_ = CharacterRenderer::normalizeID(skinID); }
void CPlayer::revive() { alive_ = true; moving_ = false; hasShield_ = false; invincibleTimer_ = 0.f; }
void CPlayer::kill() { alive_ = false; moving_ = false; }
bool CPlayer::isAlive() const { return alive_; }
bool CPlayer::isMoving() const { return moving_; }
sf::Vector2f CPlayer::getPosition() const { return { x, y }; }

void CPlayer::setPosition(sf::Vector2f pos) {
    x = pos.x; y = pos.y; moving_ = false;
}

sf::FloatRect CPlayer::getBounds() const {
    return sf::FloatRect({ x - radius_, y - radius_ }, { radius_ * 2.f, radius_ * 2.f });
}

void CPlayer::setMovementBounds(sf::FloatRect bounds) { movementBounds_ = bounds; }
void CPlayer::setCameraOffset(float cameraY) { cameraY_ = cameraY; }

void CPlayer::handleInput()
{
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
        facingDir_ = 1; moveByGridStep(-Grid::CELL_SIZE, 0.f); return;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
        facingDir_ = 2; moveByGridStep(Grid::CELL_SIZE, 0.f); return;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
        facingDir_ = 3; moveByGridStep(0.f, -Grid::CELL_SIZE); return;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
        facingDir_ = 0; moveByGridStep(0.f, Grid::CELL_SIZE); return;
    }
}

void CPlayer::handleInput(sf::Keyboard::Key key)
{
    switch (key) {
    case sf::Keyboard::Key::Left:
    case sf::Keyboard::Key::A: facingDir_ = 1; moveByGridStep(-Grid::CELL_SIZE, 0.f); break;
    case sf::Keyboard::Key::Right:
    case sf::Keyboard::Key::D: facingDir_ = 2; moveByGridStep(Grid::CELL_SIZE, 0.f); break;
    case sf::Keyboard::Key::Up:
    case sf::Keyboard::Key::W: facingDir_ = 3; moveByGridStep(0.f, -Grid::CELL_SIZE); break;
    case sf::Keyboard::Key::Down:
    case sf::Keyboard::Key::S: facingDir_ = 0; moveByGridStep(0.f, Grid::CELL_SIZE); break;
    default: break;
    }
}

void CPlayer::update(float dt) {
    if (invincibleTimer_ > 0.f) invincibleTimer_ -= dt;

    // Nếu có nhận tín hiệu di chuyển, chuyển đổi luân phiên giữa cột 0 và 2 để tạo cảm giác bước chân
    if (moving_) {
        animFrame_ = (animFrame_ == 0) ? 2 : 0;
    }
    else {
        // Đứng im thì trả về cột giữa (index 1)
        animFrame_ = 1;
    }

    moving_ = false;
}

void CPlayer::draw(sf::RenderWindow& window, float cameraY) {
    if (invincibleTimer_ > 0.f) {
        if (static_cast<int>(invincibleTimer_ * 15.f) % 2 == 0) return;
    }

    // Truyền hướng mặt (facingDir_) và nhịp bước (animFrame_) vào để vẽ ảnh
    CharacterRenderer::draw(window, skinID_, { x, y - cameraY }, radius_, facingDir_, animFrame_);
}

void CPlayer::moveByGridStep(float dx, float dy)
{
    if (!alive_) return;

    float nextX = x + dx;
    float nextY = y + dy;

    nextX = std::clamp(nextX, movementBounds_.position.x,
        movementBounds_.position.x + movementBounds_.size.x);
    if (dy == 0.f || (nextY >= movementBounds_.position.y &&
        nextY <= movementBounds_.position.y + movementBounds_.size.y))
        y = nextY;

    x = nextX;
    moving_ = true; // Bật cờ di chuyển để update() biết và đổi frame ảnh
}

bool CPlayer::isImpact(CVehicle* vehicle) {
    if (vehicle == nullptr) return false;
    return this->getBounds().findIntersection(vehicle->getBounds()).has_value();
}

bool CPlayer::isImpact(CAnimal* animal) {
    if (animal == nullptr) return false;
    return this->getBounds().findIntersection(animal->getBounds()).has_value();
}

void CPlayer::giveShield() { hasShield_ = true; }
bool CPlayer::hasShield() const { return hasShield_; }
void CPlayer::consumeShield() { hasShield_ = false; invincibleTimer_ = 1.0f; }
bool CPlayer::isInvincible() const { return invincibleTimer_ > 0.f; }