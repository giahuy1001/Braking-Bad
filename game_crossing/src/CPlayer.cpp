#include "CPlayer.h"

#include "CharacterRenderer.h"
#include "Grid.h"

#include <algorithm>

CPlayer::CPlayer(sf::Vector2f spawnPosition, int skinID, float radius)
    : spawnPosition_(spawnPosition), position_(spawnPosition), radius_(radius)
{
    setSkin(skinID);
}

void CPlayer::setSpawnPosition(sf::Vector2f pos)
{
    spawnPosition_ = pos;
}

void CPlayer::resetPosition()
{
    position_ = spawnPosition_;
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
sf::Vector2f CPlayer::getPosition() const { return position_; }

void CPlayer::setPosition(sf::Vector2f pos)
{
    // Loading a save deliberately bypasses current viewport movement bounds.
    position_ = pos;
    moving_ = false;
}

sf::FloatRect CPlayer::getBounds() const
{
    return { { position_.x - radius_, position_.y - radius_ },
             { radius_ * 2.f, radius_ * 2.f } };
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
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) { moveByGridStep(-Grid::CELL_SIZE, 0.f); return; }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) { moveByGridStep(Grid::CELL_SIZE, 0.f); return; }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) { moveByGridStep(0.f, -Grid::CELL_SIZE); return; }
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

void CPlayer::render(sf::RenderWindow& window) const
{
    CharacterRenderer::draw(window, skinID_, { position_.x, position_.y - cameraY_ }, radius_);
}

void CPlayer::moveByGridStep(float dx, float dy)
{
    if (!alive_) return;

    sf::Vector2f next = position_;
    next.x += dx;
    next.y += dy;

    // Horizontal movement is clamped; an invalid vertical step is rejected.
    // This exactly prevents the old off-screen snap at the bottom boundary.
    next.x = std::clamp(next.x, movementBounds_.position.x,
                        movementBounds_.position.x + movementBounds_.size.x);
    if (dy == 0.f || (next.y >= movementBounds_.position.y &&
                      next.y <= movementBounds_.position.y + movementBounds_.size.y))
        position_.y = next.y;
    position_.x = next.x;
    moving_ = true;
}
