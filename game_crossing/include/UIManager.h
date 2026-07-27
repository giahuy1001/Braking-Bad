#pragma once

#include <SFML/Graphics.hpp>
#include "UIState.h"
#include "Button.h"
#include "EndlessMap.h"
#include "ClassicMap.h"
#include "CharacterRenderer.h"
#include "CGameObstacle.h"
#include "persistence/SaveStore.h"
#include "persistence/RankingStore.h"
#include "persistence/ProgressStore.h"
#include "persistence/SettingsStore.h"
#include "persistence/PlayerProfile.h"
#include <vector>
#include <string>
#include <array>
#include <functional>

// ---------------------------------------------------------------------
//  Central UI controller.  Owns the window, the assets, the persistence
//  stores, the active screen, and the (optional) modal layer.
//
//  Usage from main():
//      UIManager ui(window);
//      ui.run();
//
//  Public surface is intentionally tiny; everything else is private and
//  per-state (see plan §4.8).
// ---------------------------------------------------------------------
class UIManager
{
public:
    explicit UIManager(sf::RenderWindow& window);
    ~UIManager();

    void run();                                 // main loop body

    // Loads MainMenu, Setting and Graphic from assets/theme/<season>/.
    // Returns false and keeps the currently active theme when a required
    // image is missing or cannot be decoded.
    bool setTheme(const std::string& seasonName);
    const std::string& currentTheme() const { return currentTheme_; }

private:
    // Per-frame phases
    void handleEvents();
    void update(float dt);
    void render();

    // Per-state handlers
    void handleBoot    (const sf::Event& e);
    void handleMainMenu(const sf::Event& e);
    void handleModeSel (const sf::Event& e);
    void handleName    (const sf::Event& e);
    void handleLvlSel  (const sf::Event& e);
    void handleSetting (const sf::Event& e);
    void handleGraphic (const sf::Event& e);
    void handleLoad    (const sf::Event& e);
    void handleRanking (const sf::Event& e);
    void handleHelp    (const sf::Event& e);
    void handlePlay    (const sf::Event& e);
    void handleGameOver(const sf::Event& e);
    void handlePause   (const sf::Event& e);

    // Routing helpers
    void handleModal(const sf::Event& e);
    void handleBack();                          // top-right Back icon or Esc
    void setState(UIState s);
    void rebuildButtons();                      // populates btns_ for state_
    void moveFocus(int dir);                    // -1 or +1
    void activateFocused();                     // Enter key on focused button

    // Drawing helpers
    void drawBackground();
    void drawMainMenuHoverGlow();
    void drawMainMenuDebugOverlay();
    void drawMouseDebugInfo();
    void drawBackIcon();
    void drawModalOverlay();
    void drawCenteredText(const std::string& s, float y,
                          unsigned int size = 28,
                          sf::Color color = sf::Color::White,
                          bool bold = false);

    // Text input
    static bool isValidNameChar(std::uint32_t c);
    static bool isValidName(const std::string& s);

    // Per-screen render
    void renderBoot();
    void renderMainMenu();
    void renderModeSel();
    void renderName();
    void renderLvlSel();
    void renderSetting();
    void renderGraphic();
    void renderLoad();
    void renderRanking();
    void renderHelp();
    void renderPlay();
    void renderGameOver();
    void renderPause();
    void drawMapBlock(const MapBlock& block, float cameraY);
    void drawPlayer();
    void resetGameplay();
    void updateCamera(float dt);
    float maxWalkablePlayerY() const;
    bool isEndlessPlayerOffscreen() const;
    void finishEndlessRun();
    sf::Vector2f toUiCoords(sf::Vector2i pixel) const;
    sf::Vector2f toBaseCoords(sf::Vector2i pixel) const;
    sf::FloatRect scaledMenuBounds(std::size_t index) const;
    int menuButtonAt(sf::Vector2i pixel) const;
    void activateMainMenuButton(std::size_t index);

    // Members
    sf::RenderWindow& win_;
    sf::View uiView_;
    sf::Font   font_;
    sf::Text   debugText_;
    bool       fontLoaded_ = false;
    sf::Texture bgTex_, settingBgTex_, graphicBgTex_, logoTex_, iconsTex_;
    bool       assetsLoaded_ = false;
    std::string currentTheme_ = "spring";
    bool       debugUi_ = false;

    // Coordinates are in the authored 1920x1080 image.  Adjust only these
    // values after using F3/D debug mode; rendering and collision scale them
    // automatically to the real window size.
    static const std::array<sf::FloatRect, 7> kMainMenuButtonBounds;

    UIState        state_                = UIState::Boot;
    UIState        stateBeforeModal_     = UIState::MainMenu;
    Modal          modal_                = Modal::None;
    StateContext   ctx_;
    GameSettings   cfg_;

    std::vector<Button> btns_;
    std::vector<std::function<void()>> btnActions_;
    int                 focusIdx_ = 0;
    int                 modalFocus_ = 0;          // 0 = Yes, 1 = No

    std::string nameBuffer_;
    float       bootTimer_ = 0.f;
    sf::Clock   clock_;

    // Gameplay world state. World Y decreases while moving toward the top.
    EndlessMap endlessMap_;
    ClassicMap classicMap_;
    std::vector<CObstacle*> Obstacles;
    float obstacleSpawnTimer_ = 0.f;
    float cameraY_ = -1080.f;
    sf::Vector2f playerWorldPos_;
    float elapsedPlaySec_ = 0.f;
    bool gameplayStarted_ = false; // timer/camera remain paused until first move
    bool classicWon_ = false;

    // Per-screen scratch state
    int  loadTabModeIdx_  = 0;     // 0 = Classic, 1 = Endless
    int  rankTabModeIdx_  = 0;
    int  rankScrollOffset_= 0;     // top-row index in Ranking viewport
    int  selectedSlotIdx_ = -1;    // for LoadGame delete confirmation
    bool e_isMouseOverBack = false;

    // Persistence
    SaveStore     saves_;
    RankingStore  ranks_;
    ProgressStore prog_;
    SettingsStore sets_;
};
