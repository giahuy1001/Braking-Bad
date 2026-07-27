#pragma once

#include <SFML/Graphics.hpp>

// Runtime state owned by the player, not by a screen or game manager.
enum class PlayerState
{
    Idle,
    Moving,
    Dead
};

struct PlayerMovementBounds
{
    float minX = 0.f;
    float maxX = 0.f;
    float minY = 0.f;
    float maxY = 0.f;
};

class Player
{
public:
    explicit Player(sf::Vector2f spawnPosition, float movementSpeed = 420.f,
                    float radius = 42.f);

    // Reads keyboard and the first connected controller. Call only while gameplay is active.
    void handleInput();
    void update(float deltaTime);
    void render(sf::RenderWindow& window) const;

    sf::FloatRect getBounds() const;
    void resetPosition();

    void setMovementBounds(const PlayerMovementBounds& bounds);
    void setCameraOffset(float cameraY) noexcept;
    void setPosition(sf::Vector2f position) noexcept;
    sf::Vector2f getPosition() const noexcept;
    void setSpawnPosition(sf::Vector2f spawnPosition) noexcept;

    void setHealth(int health) noexcept;
    int getHealth() const noexcept;
    void setLives(int lives) noexcept;
    int getLives() const noexcept;
    void setScore(int score) noexcept;
    int getScore() const noexcept;
    void addScore(int amount) noexcept;

    void setSkin(int skinIndex) noexcept;
    int getSkin() const noexcept;
    void setState(PlayerState state) noexcept;
    PlayerState getState() const noexcept;
    bool isMoving() const noexcept;
    bool isDead() const noexcept;
    void kill() noexcept;
    void revive() noexcept;

private:
    void refreshVisual();

    sf::Vector2f spawnPosition_;
    sf::Vector2f position_;
    sf::Vector2f movement_;
    PlayerMovementBounds movementBounds_;
    float cameraY_ = 0.f;
    float movementSpeed_;
    float radius_;
    int health_ = 1;
    int lives_ = 1;
    int score_ = 0;
    int skinIndex_ = 1;
    PlayerState state_ = PlayerState::Idle;
    sf::CircleShape visual_;
};
