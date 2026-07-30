#include "CPlayer.h"
#include "CharacterRenderer.h"
#include "Grid.h"
#include <algorithm>
#include <cmath>

CPlayer::CPlayer(sf::Vector2f spawnPosition, int skinID, float radius)
    : CGameObject(spawnPosition.x, spawnPosition.y, radius * 2.f, radius * 2.f),
    spawnPosition_(spawnPosition), radius_(radius),
    targetX_(spawnPosition.x), targetY_(spawnPosition.y)
{
    setSkin(skinID);
}

void CPlayer::setSpawnPosition(sf::Vector2f pos) { spawnPosition_ = pos; }

void CPlayer::resetPosition() {
    x = spawnPosition_.x; y = spawnPosition_.y;
    targetX_ = x; targetY_ = y;
    moving_ = false; hasShield_ = false; invincibleTimer_ = 0.f;
    facingDir_ = 0; animFrame_ = 1; animTimer_ = 0.f;
    revive();
}

void CPlayer::setSkin(int skinID) { skinID_ = CharacterRenderer::normalizeID(skinID); }
void CPlayer::revive() { alive_ = true; moving_ = false; hasShield_ = false; invincibleTimer_ = 0.f; }
void CPlayer::kill() { alive_ = false; moving_ = false; }
bool CPlayer::isAlive() const { return alive_; }
bool CPlayer::isMoving() const { return moving_; }
sf::Vector2f CPlayer::getPosition() const { return { x, y }; }

void CPlayer::setPosition(sf::Vector2f pos) {
    x = pos.x; y = pos.y;
    targetX_ = x; targetY_ = y;
    moving_ = false;
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

void CPlayer::moveByGridStep(float dx, float dy)
{
    if (!alive_) return;

    // Khoá phím: Chỉ cho phép bước tiếp khi đã đến (hoặc sắp đến) đích của bước trước đó
    if (std::abs(x - targetX_) > 5.f || std::abs(y - targetY_) > 5.f) return;

    float nextX = targetX_ + dx;
    float nextY = targetY_ + dy;

    nextX = std::clamp(nextX, movementBounds_.position.x,
        movementBounds_.position.x + movementBounds_.size.x);

    if (dy == 0.f || (nextY >= movementBounds_.position.y &&
        nextY <= movementBounds_.position.y + movementBounds_.size.y)) {
        targetY_ = nextY;
    }

    targetX_ = nextX;
    moving_ = true;
}

void CPlayer::update(float dt) {
    if (invincibleTimer_ > 0.f) invincibleTimer_ -= dt;

    bool isMovingNow = false;

    // 1. DI CHUYỂN MƯỢT (Nội suy trục X và Y tiến dần về đích)
    if (std::abs(x - targetX_) > 0.1f || std::abs(y - targetY_) > 0.1f) {
        isMovingNow = true;

        if (x < targetX_) {
            x += moveSpeed_ * dt;
            if (x > targetX_) x = targetX_; // Tránh đi quá lố
        }
        else if (x > targetX_) {
            x -= moveSpeed_ * dt;
            if (x < targetX_) x = targetX_;
        }

        if (y < targetY_) {
            y += moveSpeed_ * dt;
            if (y > targetY_) y = targetY_;
        }
        else if (y > targetY_) {
            y -= moveSpeed_ * dt;
            if (y < targetY_) y = targetY_;
        }
    }
    else {
        moving_ = false;
    }

    // 2. LÀM CHẬM ANIMATION
    if (isMovingNow) {
        animTimer_ += dt;
        // Cứ mỗi 0.1 giây mới nhấc chân 1 lần (tạo cảm giác đi bộ chân thật)
        if (animTimer_ >= 0.1f) {
            animFrame_ = (animFrame_ == 0) ? 2 : 0;
            animTimer_ = 0.f;
        }
    }
    else {
        animFrame_ = 1; // Đứng im chụm chân
        animTimer_ = 0.f;
    }
}

void CPlayer::draw(sf::RenderWindow& window, float cameraY) {
    if (invincibleTimer_ > 0.f) {
        if (static_cast<int>(invincibleTimer_ * 15.f) % 2 == 0) return;
    }

    CharacterRenderer::draw(window, skinID_, { x, y - cameraY }, radius_, facingDir_, animFrame_);
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