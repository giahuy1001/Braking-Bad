#pragma once

#include <SFML/Graphics.hpp>
#include "AudioManager.h"
#include "UIState.h"
#include "Button.h"
#include "EndlessMap.h"
#include "ClassicMap.h"
#include "CharacterRenderer.h"
#include "MapBackground.h"
#include "CPlayer.h"
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
#include <optional>

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
    void drawActiveDebugHitboxes();
    void drawMouseDebugInfo();
    void drawDebugAudioMixer();
    void drawBackIcon();
    void drawModalOverlay();
    void drawGameplaySidebarHitboxes();
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
    std::string mapImageKey(const MapBlock& block) const;
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
    sf::FloatRect scaledBaseRect(const sf::FloatRect& baseRect) const;
    void setSfxVolumeFromMouse(sf::Vector2i pixel);
    void setMusicVolumeFromMouse(sf::Vector2i pixel);
    void applyAudioVolumes();
    void savePausedRunAndExit();
    bool loadGraphicPreviewThemeIcon();
    void beginGraphicPreview();
    bool commitGraphicPreview();
    bool commitPreviewTheme();
    void commitPreviewCharacter();
    void capturePausedFrame();
    sf::FloatRect pauseOverlayBounds() const;
    sf::FloatRect pauseResumeBounds() const;
    sf::FloatRect pauseOutBounds() const;
    bool handleDebugAudioMixerKey(const sf::Event::KeyPressed& key);
    sf::FloatRect sidebarPauseBounds() const;
    sf::FloatRect sidebarSaveBounds() const;
    sf::FloatRect sidebarQuitBounds() const;
    sf::FloatRect confirmationPopupBounds() const;
    sf::FloatRect confirmationYesBounds() const;
    sf::FloatRect confirmationNoBounds() const;
    void saveCurrentRun();
    void startClassicLevel(int level);

    // Members
    sf::RenderWindow& win_;
    sf::View uiView_;
    sf::Font   font_;
    sf::Text   debugText_;
    bool       fontLoaded_ = false;
    sf::Texture bgTex_, settingBgTex_, graphicBgTex_, loadBgTex_, rankingBgTex_, iconThemeTex_, pauseTex_, backButtonTex_,
                settingButtonTex_, logoTex_, iconsTex_;
    sf::Texture scoreTableTex_, saveTex_, quitTex_;
    sf::Texture levelBgTex_;
    sf::Font   rankingFont_;
    sf::Texture pauseFrameTex_;
    MapBackground mapBackground_;
    bool       assetsLoaded_ = false;
    bool       pauseAssetLoaded_ = false;
    bool       scoreTableAssetLoaded_ = false;
    bool       saveAssetLoaded_ = false;
    bool       quitAssetLoaded_ = false;
    bool       levelBgAssetLoaded_ = false;
    bool       pauseFrameValid_ = false;
    std::string currentTheme_ = "winter";
    bool       debugUi_ = false;
    bool       debugAudioMixer_ = false;
    std::size_t selectedAudioCategory_ = 0;
    int        currentThemeIndex_ = 0;
    int        previewThemeIndex_ = 0;
    int        previewCharacterId_ = 1;
    bool       draggingSfx_ = false;
    bool       draggingMusic_ = false;

    // Coordinates are in the authored 1920x1080 image.  Adjust only these
    // values after using F3/D debug mode; rendering and collision scale them
    // automatically to the real window size.
    static const std::array<sf::FloatRect, 7> kMainMenuButtonBounds;
    static const std::array<sf::FloatRect, 10> kLevelButtonBounds;
    static const std::array<std::string, 4> kThemeNames;
    static const sf::FloatRect kCharacterPanelBounds, kCharacterPrevBounds, kCharacterNextBounds;
    static const sf::FloatRect kThemePanelBounds, kThemePrevBounds, kThemeNextBounds;
    static const sf::FloatRect kSfxTrackBounds, kSfxDecBounds, kSfxIncBounds;
    static const sf::FloatRect kMusicTrackBounds, kMusicDecBounds, kMusicIncBounds, kSettingOkBounds;
    static const sf::FloatRect kGraphicOkThemeBounds;
    static const sf::FloatRect kGraphicCharacterOkBounds;

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
    sf::Clock   clock_; // Restarted at Pause/Resume boundaries to discard stale dt.

    // Gameplay world state. World Y decreases while moving toward the top.
    EndlessMap endlessMap_;
    ClassicMap classicMap_;
    std::vector<CObstacle*> Obstacles;
    float obstacleSpawnTimer_ = 0.f;
    float cameraY_ = -1080.f;
    CPlayer player_;
    float elapsedPlaySec_ = 0.f;
    bool gameplayStarted_ = false; // timer/camera remain paused until first move
    bool classicWon_ = false;

    enum class TrafficLight { Green, Yellow, Red };
    TrafficLight currentLight_ = TrafficLight::Green;
    float trafficLightTimer_ = 0.f;
    sf::Texture texTrafficGreen_, texTrafficYellow_, texTrafficRed_;
    sf::Texture texShieldItem_;
    sf::Sprite trafficLightSprite_;

    // Per-screen scratch state
    int  loadTabModeIdx_  = 0;     // 0 = Classic, 1 = Endless
    int  selectedSlotIdx_ = -1;    // for LoadGame delete confirmation
    bool e_isMouseOverBack = false;

    // Persistence
    SaveStore     saves_;
    RankingStore  ranks_;
    ProgressStore prog_;
    SettingsStore sets_;

    // AudioManager giu toan bo asset va cong thuc mix; UI chi gui volume
    // nguoi dung va input cua Debug Audio Mixer.
    AudioManager audio_;
};
