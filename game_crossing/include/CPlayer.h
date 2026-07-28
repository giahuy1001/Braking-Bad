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

    // Polling entry point for callers that do not have event-level input.
    void handleInput();
    // Event-level input retains the game's one-grid-cell-per-keypress rule.
    void handleInput(sf::Keyboard::Key key);
    void update(float dt);

    // (Replaced the friends' "render" function to properly match CGameObject)
    void draw(sf::RenderWindow& window, float cameraY) override;

    // COLLISION DETECTION
    bool isImpact(CVehicle* vehicle);
    bool isImpact(CAnimal* animal);

private:
    void moveByGridStep(float dx, float dy);

    sf::Vector2f spawnPosition_;

    sf::FloatRect movementBounds_;
    float cameraY_ = 0.f;
    float radius_ = 42.f;
    int skinID_ = 1;
    bool alive_ = true;
    bool moving_ = false;
};