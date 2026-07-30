#pragma once
#include "CGameObject.h"
#include "CVehicle.h"
#include "CAnimal.h"
#include <SFML/Graphics.hpp>

class CPlayer : public CGameObject
{
public:
    explicit CPlayer(sf::Vector2f spawnPosition = { 0.f, 0.f }, int skinID = 1,
        float radius = 42.f);

    void setSpawnPosition(sf::Vector2f pos);
    void resetPosition();
    void setSkin(int skinID);
    void revive();
    void kill();

    bool isAlive() const;
    bool isMoving() const;

    sf::Vector2f getPosition() const;
    sf::FloatRect getBounds() const;
    void setPosition(sf::Vector2f pos);

    void setMovementBounds(sf::FloatRect bounds);
    void setCameraOffset(float cameraY);

    void handleInput();
    void handleInput(sf::Keyboard::Key key);
    void update(float dt);

    void draw(sf::RenderWindow& window, float cameraY) override;

    bool isImpact(CVehicle* vehicle);
    bool isImpact(CAnimal* animal);

    void giveShield();
    bool hasShield() const;
    void consumeShield();
    bool isInvincible() const;

private:
    void moveByGridStep(float dx, float dy);

    sf::Vector2f spawnPosition_;
    sf::FloatRect movementBounds_;
    float cameraY_ = 0.f;
    float radius_ = 42.f;
    int skinID_ = 1;
    bool alive_ = true;
    bool moving_ = false;

    bool hasShield_ = false;
    float invincibleTimer_ = 0.f;

    // --- ANIMATION STATE ---
    int facingDir_ = 0; // 0: Down, 1: Left, 2: Right, 3: Up
    int animFrame_ = 1; // Cột 1 là thế đứng im
};