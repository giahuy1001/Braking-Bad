#include "Player.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float ControllerDeadZone = 25.f;

    sf::Color skinColor(int skin)
    {
        switch (((skin - 1) % 6 + 6) % 6 + 1) {
        case 1: return sf::Color::Blue;
        case 2: return sf::Color::Red;
        case 3: return sf::Color::Yellow;
        case 4: return sf::Color::Green;
        case 5: return sf::Color::Magenta;
        default: return sf::Color(255, 145, 0);
        }
    }
}

Player::Player(sf::Vector2f spawnPosition, float movementSpeed, float radius)
    : spawnPosition_(spawnPosition), position_(spawnPosition), movementSpeed_(movementSpeed),
      radius_(radius), visual_(radius)
{
    visual_.setOrigin({ radius_, radius_ });
    refreshVisual();
}

void Player::handleInput()
{
    if (isDead()) {
        movement_ = {};
        return;
    }

    sf::Vector2f input;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) input.x -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) input.x += 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) input.y -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) input.y += 1.f;

    if (sf::Joystick::isConnected(0)) {
        const float x = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::X);
        const float y = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::Y);
        if (std::abs(x) > ControllerDeadZone) input.x += x / 100.f;
        if (std::abs(y) > ControllerDeadZone) input.y += y / 100.f;
    }

    const float length = std::sqrt(input.x * input.x + input.y * input.y);
    movement_ = length > 0.f ? input / length : sf::Vector2f{};
    state_ = length > 0.f ? PlayerState::Moving : PlayerState::Idle;
}

void Player::update(float deltaTime)
{
    if (isDead() || deltaTime <= 0.f) return;

    position_ += movement_ * movementSpeed_ * deltaTime;
    position_.x = std::clamp(position_.x, movementBounds_.minX, movementBounds_.maxX);
    position_.y = std::clamp(position_.y, movementBounds_.minY, movementBounds_.maxY);
    visual_.setPosition({ position_.x, position_.y - cameraY_ });
}

void Player::render(sf::RenderWindow& window) const
{
    window.draw(visual_);
}

sf::FloatRect Player::getBounds() const
{
    return { { position_.x - radius_, position_.y - radius_ }, { radius_ * 2.f, radius_ * 2.f } };
}

void Player::resetPosition()
{
    position_ = spawnPosition_;
    movement_ = {};
    state_ = PlayerState::Idle;
    visual_.setPosition({ position_.x, position_.y - cameraY_ });
}

void Player::setMovementBounds(const PlayerMovementBounds& bounds) { movementBounds_ = bounds; }
void Player::setCameraOffset(float cameraY) noexcept { cameraY_ = cameraY; visual_.setPosition({ position_.x, position_.y - cameraY_ }); }
void Player::setPosition(sf::Vector2f position) noexcept { position_ = position; visual_.setPosition({ position_.x, position_.y - cameraY_ }); }
sf::Vector2f Player::getPosition() const noexcept { return position_; }
void Player::setSpawnPosition(sf::Vector2f spawnPosition) noexcept { spawnPosition_ = spawnPosition; }
void Player::setHealth(int health) noexcept { health_ = std::max(0, health); if (health_ == 0) kill(); }
int Player::getHealth() const noexcept { return health_; }
void Player::setLives(int lives) noexcept { lives_ = std::max(0, lives); }
int Player::getLives() const noexcept { return lives_; }
void Player::setScore(int score) noexcept { score_ = std::max(0, score); }
int Player::getScore() const noexcept { return score_; }
void Player::addScore(int amount) noexcept { setScore(score_ + amount); }
void Player::setSkin(int skinIndex) noexcept { skinIndex_ = skinIndex; refreshVisual(); }
int Player::getSkin() const noexcept { return skinIndex_; }
void Player::setState(PlayerState state) noexcept { state_ = state; if (state_ == PlayerState::Dead) movement_ = {}; }
PlayerState Player::getState() const noexcept { return state_; }
bool Player::isMoving() const noexcept { return state_ == PlayerState::Moving; }
bool Player::isDead() const noexcept { return state_ == PlayerState::Dead; }
void Player::kill() noexcept { state_ = PlayerState::Dead; movement_ = {}; }
void Player::revive() noexcept { if (health_ <= 0) health_ = 1; state_ = PlayerState::Idle; }

void Player::refreshVisual()
{
    visual_.setFillColor(skinColor(skinIndex_));
    visual_.setOutlineThickness(2.f);
    visual_.setOutlineColor(sf::Color::Black);
}
