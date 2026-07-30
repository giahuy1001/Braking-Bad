/**
 * @file UIManager.h
 * @brief Central controller for the game user interface, screen state machine, UI assets, and UI-owned services.
 */
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

/**
 * @brief Coordinates the complete game user interface.
 *
 * UIManager owns the screen state machine, translates input into UI actions,
 * and keeps seasonal artwork, settings, persistence, and gameplay presentation synchronized.
 */
class UIManager
{
public:
    /**
     * @brief Performs the uimanager operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     */
    explicit UIManager(sf::RenderWindow& window);
    ~UIManager();

    /**
     * @brief Executes one complete UI lifecycle until the application closes.
     */
    void run();

    /**
     * @brief Loads a complete seasonal asset set atomically so a failed asset never leaves a mixed theme on screen.
     * @param Input required by this UI operation.
     * @return Result produced by this UI operation.
     */
    bool setTheme(const std::string& seasonName);
    const std::string& currentTheme() const { return currentTheme_; }

private:

    /**
     * @brief Routes queued input to exactly one state handler so modal input cannot leak into the underlying screen.
     */
    void handleEvents();
    /**
     * @brief Advances state that depends on elapsed time after input has established the current intent.
     * @param Input required by this UI operation.
     */
    void update(float dt);
    /**
     * @brief Draws the active screen before any modal, preserving the modal as the topmost interaction layer.
     */
    void render();

    /**
     * @brief Handles input for the startup loading screen.
     * @param Input required by this UI operation.
     */
    void handleLoading (const sf::Event& e);
    /**
     * @brief Performs the handle main menu operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     */
    void handleMainMenu(const sf::Event& e);
    /**
     * @brief Performs the handle mode sel operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     */
    void handleModeSel (const sf::Event& e);
    /**
     * @brief Performs the handle name operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     */
    void handleName    (const sf::Event& e);
    /**
     * @brief Performs the handle lvl sel operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     */
    void handleLvlSel  (const sf::Event& e);
    /**
     * @brief Performs the handle setting operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     */
    void handleSetting (const sf::Event& e);
    /**
     * @brief Performs the handle graphic operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     */
    void handleGraphic (const sf::Event& e);
    /**
     * @brief Performs the handle load operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     */
    void handleLoad    (const sf::Event& e);
    /**
     * @brief Performs the handle ranking operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     */
    void handleRanking (const sf::Event& e);
    /**
     * @brief Performs the handle help operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     */
    void handleHelp    (const sf::Event& e);
    /**
     * @brief Performs the handle play operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     */
    void handlePlay    (const sf::Event& e);
    /**
     * @brief Performs the handle game over operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     */
    void handleGameOver(const sf::Event& e);
    /**
     * @brief Performs the handle pause operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     */
    void handlePause   (const sf::Event& e);

    /**
     * @brief Consumes dialog input before state input to prevent an action from being applied twice.
     * @param Input required by this UI operation.
     */
    void handleModal(const sf::Event& e);
    /**
     * @brief Performs the handle back operation while preserving the current UI state invariants.
     */
    void handleBack();
    /**
     * @brief Changes the screen through one path to keep focus, controls, and transition context synchronized.
     * @param Input required by this UI operation.
     */
    void setState(UIState s);
    /**
     * @brief Builds controls after each transition so callbacks always belong to the active screen.
     */
    void rebuildButtons();
    /**
     * @brief Performs the move focus operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     */
    void moveFocus(int dir);
    /**
     * @brief Performs the activate focused operation while preserving the current UI state invariants.
     */
    void activateFocused();

    /**
     * @brief Performs the draw background operation while preserving the current UI state invariants.
     */
    void drawBackground();
    /**
     * @brief Performs the draw main menu hover glow operation while preserving the current UI state invariants.
     */
    void drawMainMenuHoverGlow();
    /**
     * @brief Performs the draw main menu debug overlay operation while preserving the current UI state invariants.
     */
    void drawMainMenuDebugOverlay();
    /**
     * @brief Performs the draw active debug hitboxes operation while preserving the current UI state invariants.
     */
    void drawActiveDebugHitboxes();
    /**
     * @brief Performs the draw mouse debug info operation while preserving the current UI state invariants.
     */
    void drawMouseDebugInfo();
    /**
     * @brief Performs the draw debug audio mixer operation while preserving the current UI state invariants.
     */
    void drawDebugAudioMixer();
    /**
     * @brief Performs the draw back icon operation while preserving the current UI state invariants.
     */
    void drawBackIcon();
    /**
     * @brief Performs the draw modal overlay operation while preserving the current UI state invariants.
     */
    void drawModalOverlay();
    /**
     * @brief Performs the draw gameplay sidebar hitboxes operation while preserving the current UI state invariants.
     */
    void drawGameplaySidebarHitboxes();
    void drawCenteredText(const std::string& s, float y,
                          unsigned int size = 28,
                          sf::Color color = sf::Color::White,
    ///< Internal storage used to coordinate UI state and rendering.
                          bool bold = false);

    /**
     * @brief Performs the is valid name char operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     * @return Result produced by this UI operation.
     */
    static bool isValidNameChar(std::uint32_t c);
    /**
     * @brief Performs the is valid name operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     * @return Result produced by this UI operation.
     */
    static bool isValidName(const std::string& s);

    /**
     * @brief Draws the loading artwork, blinking prompt, and zoom transition.
     */
    void renderLoading();
    /**
     * @brief Performs the render main menu operation while preserving the current UI state invariants.
     */
    void renderMainMenu();
    /**
     * @brief Performs the render mode sel operation while preserving the current UI state invariants.
     */
    void renderModeSel();
    /**
     * @brief Performs the render name operation while preserving the current UI state invariants.
     */
    void renderName();
    /**
     * @brief Performs the render lvl sel operation while preserving the current UI state invariants.
     */
    void renderLvlSel();
    /**
     * @brief Performs the render setting operation while preserving the current UI state invariants.
     */
    void renderSetting();
    /**
     * @brief Performs the render graphic operation while preserving the current UI state invariants.
     */
    void renderGraphic();
    /**
     * @brief Performs the render load operation while preserving the current UI state invariants.
     */
    void renderLoad();
    /**
     * @brief Performs the render ranking operation while preserving the current UI state invariants.
     */
    void renderRanking();
    /**
     * @brief Performs the render help operation while preserving the current UI state invariants.
     */
    void renderHelp();
    /**
     * @brief Draws gameplay in layer order so world actors remain behind HUD and sidebar controls.
     */
    void renderPlay();
    /**
     * @brief Performs the render game over operation while preserving the current UI state invariants.
     */
    void renderGameOver();
    /**
     * @brief Performs the render pause operation while preserving the current UI state invariants.
     */
    void renderPause();
    /**
     * @brief Renders a map block with the camera offset while collision continues to use stable world coordinates.
     * @param Input required by this UI operation.
     */
    void drawMapBlock(const MapBlock& block, float cameraY);
    /**
     * @brief Uses the block-selected artwork key to keep rendered lanes aligned with the generated map data.
     * @param Input required by this UI operation.
     * @return Result produced by this UI operation.
     */
    std::string mapImageKey(const MapBlock& block) const;
    /**
     * @brief Performs the draw player operation while preserving the current UI state invariants.
     */
    void drawPlayer();
    /**
     * @brief Performs the reset gameplay operation while preserving the current UI state invariants.
     */
    void resetGameplay();
    /**
     * @brief Keeps the player readable on screen while allowing world coordinates to remain independent of the viewport.
     * @param Input required by this UI operation.
     */
    void updateCamera(float dt);
    /**
     * @brief Performs the max walkable player y operation while preserving the current UI state invariants.
     * @return Result produced by this UI operation.
     */
    float maxWalkablePlayerY() const;
    /**
     * @brief Performs the is endless player offscreen operation while preserving the current UI state invariants.
     * @return Result produced by this UI operation.
     */
    bool isEndlessPlayerOffscreen() const;
    /**
     * @brief Performs the finish endless run operation while preserving the current UI state invariants.
     */
    void finishEndlessRun();
    /** Stores the completed run once, then returns the result panel to Main Menu. */
    void returnFromGameOver();
    /**
     * @brief Converts pixels through the UI view so input follows letterboxing and view scaling.
     * @param Input required by this UI operation.
     * @return Result produced by this UI operation.
     */
    sf::Vector2f toUiCoords(sf::Vector2i pixel) const;
    /**
     * @brief Converts pixels into authored-art coordinates so one set of hitboxes works at every window size.
     * @param Input required by this UI operation.
     * @return Result produced by this UI operation.
     */
    sf::Vector2f toBaseCoords(sf::Vector2i pixel) const;
    /**
     * @brief Performs the scaled menu bounds operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     * @return Result produced by this UI operation.
     */
    sf::FloatRect scaledMenuBounds(std::size_t index) const;
    /**
     * @brief Performs the menu button at operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     * @return Result produced by this UI operation.
     */
    int menuButtonAt(sf::Vector2i pixel) const;
    /**
     * @brief Performs the activate main menu button operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     */
    void activateMainMenuButton(std::size_t index);
    /**
     * @brief Scales authored hitboxes with the artwork, preventing click targets from drifting after a resize.
     * @param Input required by this UI operation.
     * @return Result produced by this UI operation.
     */
    sf::FloatRect scaledBaseRect(const sf::FloatRect& baseRect) const;
    /**
     * @brief Performs the set sfx volume from mouse operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     */
    void setSfxVolumeFromMouse(sf::Vector2i pixel);
    /**
     * @brief Performs the set music volume from mouse operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     */
    void setMusicVolumeFromMouse(sf::Vector2i pixel);
    /**
     * @brief Applies both volume categories together so persisted settings and live audio remain consistent.
     */
    void applyAudioVolumes();
    /**
     * @brief Performs the save paused run and exit operation while preserving the current UI state invariants.
     */
    void savePausedRunAndExit();
    /**
     * @brief Performs the load graphic preview theme icon operation while preserving the current UI state invariants.
     * @return Result produced by this UI operation.
     */
    bool loadGraphicPreviewThemeIcon();
    /**
     * @brief Performs the begin graphic preview operation while preserving the current UI state invariants.
     */
    void beginGraphicPreview();
    /**
     * @brief Performs the commit graphic preview operation while preserving the current UI state invariants.
     * @return Result produced by this UI operation.
     */
    bool commitGraphicPreview();
    /**
     * @brief Performs the commit preview theme operation while preserving the current UI state invariants.
     * @return Result produced by this UI operation.
     */
    bool commitPreviewTheme();
    /**
     * @brief Performs the commit preview character operation while preserving the current UI state invariants.
     */
    void commitPreviewCharacter();
    /**
     * @brief Performs the capture paused frame operation while preserving the current UI state invariants.
     */
    void capturePausedFrame();
    /** Captures the last gameplay frame once before displaying a result panel. */
    void captureResultFrame();
    /**
     * @brief Performs the pause overlay bounds operation while preserving the current UI state invariants.
     * @return Result produced by this UI operation.
     */
    sf::FloatRect pauseOverlayBounds() const;
    /**
     * @brief Performs the pause resume bounds operation while preserving the current UI state invariants.
     * @return Result produced by this UI operation.
     */
    sf::FloatRect pauseResumeBounds() const;
    /**
     * @brief Performs the pause out bounds operation while preserving the current UI state invariants.
     * @return Result produced by this UI operation.
     */
    sf::FloatRect pauseOutBounds() const;
    /**
     * @brief Performs the handle debug audio mixer key operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     * @return Result produced by this UI operation.
     */
    bool handleDebugAudioMixerKey(const sf::Event::KeyPressed& key);
    /**
     * @brief Performs the sidebar pause bounds operation while preserving the current UI state invariants.
     * @return Result produced by this UI operation.
     */
    sf::FloatRect sidebarPauseBounds() const;
    /**
     * @brief Performs the sidebar save bounds operation while preserving the current UI state invariants.
     * @return Result produced by this UI operation.
     */
    sf::FloatRect sidebarSaveBounds() const;
    /**
     * @brief Performs the sidebar quit bounds operation while preserving the current UI state invariants.
     * @return Result produced by this UI operation.
     */
    sf::FloatRect sidebarQuitBounds() const;
    /**
     * @brief Performs the confirmation popup bounds operation while preserving the current UI state invariants.
     * @return Result produced by this UI operation.
     */
    sf::FloatRect confirmationPopupBounds() const;
    /**
     * @brief Performs the confirmation yes bounds operation while preserving the current UI state invariants.
     * @return Result produced by this UI operation.
     */
    sf::FloatRect confirmationYesBounds() const;
    /**
     * @brief Performs the confirmation no bounds operation while preserving the current UI state invariants.
     * @return Result produced by this UI operation.
     */
    sf::FloatRect confirmationNoBounds() const;
    /**
     * @brief Performs the save current run operation while preserving the current UI state invariants.
     */
    void saveCurrentRun();
    /**
     * @brief Performs the start classic level operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     */
    void startClassicLevel(int level);
    /**
     * @brief Performs the confirm name operation while preserving the current UI state invariants.
     */
    void confirmName();

    /**
     * @brief Performs the select game mode operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     */
    void selectGameMode(StateContext::Mode mode);

    /**
     * @brief Performs the game mode screen bounds operation while preserving the current UI state invariants.
     * @return Result produced by this UI operation.
     */
    sf::FloatRect gameModeScreenBounds() const;

    /**
     * @brief Performs the game mode button bounds operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     * @return Result produced by this UI operation.
     */
    sf::FloatRect gameModeButtonBounds(std::size_t index) const;
    /** Returns the original-size active Win/Lose panel centered in the window. */
    sf::FloatRect resultScreenBounds() const;
    /** Returns the artwork-local HOME button as a physical invisible hitbox. */
    sf::FloatRect resultHomeBounds() const;
    /**
     * @brief Performs the level screen bounds operation while preserving the current UI state invariants.
     * @return Result produced by this UI operation.
     */
    sf::FloatRect levelScreenBounds() const;
    /**
     * @brief Performs the user name screen bounds operation while preserving the current UI state invariants.
     * @return Result produced by this UI operation.
     */
    sf::FloatRect userNameScreenBounds() const;
    /**
     * @brief Performs the level button bounds operation while preserving the current UI state invariants.
     * @param Input required by this UI operation.
     * @return Result produced by this UI operation.
     */
    sf::FloatRect levelButtonBounds(std::size_t index) const;
    /**
     * @brief Performs the user name input bounds operation while preserving the current UI state invariants.
     * @return Result produced by this UI operation.
     */
    sf::FloatRect userNameInputBounds() const;
    /**
     * @brief Performs the user name confirm bounds operation while preserving the current UI state invariants.
     * @return Result produced by this UI operation.
     */
    sf::FloatRect userNameConfirmBounds() const;

    ///< Internal storage used to coordinate UI state and rendering.
    sf::RenderWindow& win_;
    ///< Internal storage used to coordinate UI state and rendering.
    sf::View uiView_;
    ///< Internal storage used to coordinate UI state and rendering.
    sf::Font   font_;
    ///< Internal storage used to coordinate UI state and rendering.
    sf::Text   debugText_;
    ///< Internal storage used to coordinate UI state and rendering.
    bool       fontLoaded_ = false;
    sf::Texture bgTex_, settingBgTex_, graphicBgTex_, loadBgTex_, rankingBgTex_, iconThemeTex_, pauseTex_, backButtonTex_,
    ///< Internal storage used to coordinate UI state and rendering.
                settingButtonTex_, logoTex_, iconsTex_, loadingTex_;
    ///< Internal storage used to coordinate UI state and rendering.
    sf::Texture scoreTableTex_, saveTex_, quitTex_;

    ///< Internal storage used to coordinate UI state and rendering.
    sf::Texture levelBgTex_, userNameBgTex_, gameModeBgTex_, winTex_, loseTex_;
    ///< Internal storage used to coordinate UI state and rendering.
    sf::Font   rankingFont_;
    ///< Internal storage used to coordinate UI state and rendering.
    sf::Texture pauseFrameTex_;
    sf::Texture resultFrameTex_;
    ///< Internal storage used to coordinate UI state and rendering.
    MapBackground mapBackground_;
    ///< Internal storage used to coordinate UI state and rendering.
    bool       assetsLoaded_ = false;
    ///< Internal storage used to coordinate UI state and rendering.
    bool       pauseAssetLoaded_ = false;
    ///< Internal storage used to coordinate UI state and rendering.
    bool       scoreTableAssetLoaded_ = false;
    ///< Internal storage used to coordinate UI state and rendering.
    bool       saveAssetLoaded_ = false;
    ///< Internal storage used to coordinate UI state and rendering.
    bool       quitAssetLoaded_ = false;
    ///< Internal storage used to coordinate UI state and rendering.
    bool       levelBgAssetLoaded_ = false;
    ///< Internal storage used to coordinate UI state and rendering.
    bool       userNameBgAssetLoaded_ = false;
    ///< Internal storage used to coordinate UI state and rendering.
    bool       gameModeBgAssetLoaded_ = false;
    bool       winAssetLoaded_ = false;
    bool       loseAssetLoaded_ = false;
    ///< Internal storage used to coordinate UI state and rendering.
    bool       pauseFrameValid_ = false;
    bool       resultFrameValid_ = false;
    ///< Internal storage used to coordinate UI state and rendering.
    std::string currentTheme_ = "winter";
    ///< Internal storage used to coordinate UI state and rendering.
    bool       debugUi_ = false;
    ///< Internal storage used to coordinate UI state and rendering.
    bool       debugAudioMixer_ = false;
    ///< Internal storage used to coordinate UI state and rendering.
    std::size_t selectedAudioCategory_ = 0;
    ///< Internal storage used to coordinate UI state and rendering.
    int        currentThemeIndex_ = 0;
    ///< Internal storage used to coordinate UI state and rendering.
    int        previewThemeIndex_ = 0;
    ///< Internal storage used to coordinate UI state and rendering.
    int        previewCharacterId_ = 1;
    ///< Internal storage used to coordinate UI state and rendering.
    bool       draggingSfx_ = false;
    ///< Internal storage used to coordinate UI state and rendering.
    bool       draggingMusic_ = false;

    ///< Internal storage used to coordinate UI state and rendering.
    static const std::array<sf::FloatRect, 7> kMainMenuButtonBounds;
    ///< Internal storage used to coordinate UI state and rendering.
    static const std::array<sf::FloatRect, 10> kLevelButtonBounds;

    ///< Internal storage used to coordinate UI state and rendering.
    static const std::array<sf::FloatRect, 2> kGameModeButtonBounds;
    /// Original Win.png/Lose.png-local bounds for their shared HOME button.
    static const sf::FloatRect kResultHomeButtonBounds;
    ///< Internal storage used to coordinate UI state and rendering.
    static const std::array<std::string, 4> kThemeNames;
    ///< Internal storage used to coordinate UI state and rendering.
    static const sf::FloatRect kCharacterPanelBounds, kCharacterPrevBounds, kCharacterNextBounds;
    ///< Internal storage used to coordinate UI state and rendering.
    static const sf::FloatRect kThemePanelBounds, kThemePrevBounds, kThemeNextBounds;
    ///< Internal storage used to coordinate UI state and rendering.
    static const sf::FloatRect kSfxTrackBounds, kSfxDecBounds, kSfxIncBounds;
    ///< Internal storage used to coordinate UI state and rendering.
    static const sf::FloatRect kMusicTrackBounds, kMusicDecBounds, kMusicIncBounds, kSettingOkBounds;
    ///< Internal storage used to coordinate UI state and rendering.
    static const sf::FloatRect kGraphicOkThemeBounds;
    ///< Internal storage used to coordinate UI state and rendering.
    static const sf::FloatRect kGraphicCharacterOkBounds;

    ///< Internal storage used to coordinate UI state and rendering.
    UIState        state_                = UIState::Loading;
    ///< Internal storage used to coordinate UI state and rendering.
    UIState        stateBeforeModal_     = UIState::MainMenu;
    ///< Internal storage used to coordinate UI state and rendering.
    Modal          modal_                = Modal::None;
    ///< Internal storage used to coordinate UI state and rendering.
    StateContext   ctx_;
    ///< Internal storage used to coordinate UI state and rendering.
    GameSettings   cfg_;

    ///< Internal storage used to coordinate UI state and rendering.
    std::vector<Button> btns_;
    std::vector<std::function<void()>> btnActions_;
    ///< Internal storage used to coordinate UI state and rendering.
    int                 focusIdx_ = 0;
    ///< Internal storage used to coordinate UI state and rendering.
    int                 modalFocus_ = 0;

    ///< Internal storage used to coordinate UI state and rendering.
    std::string nameBuffer_;
    ///< Internal storage used to coordinate UI state and rendering.
    sf::View     loadingView_;
    float        loadingBlinkElapsed_ = 0.f;
    float        loadingZoomElapsed_ = 0.f;
    bool         loadingTransitionActive_ = false;
    ///< Internal storage used to coordinate UI state and rendering.
    sf::Clock   clock_;

    ///< Internal storage used to coordinate UI state and rendering.
    EndlessMap endlessMap_;
    ///< Internal storage used to coordinate UI state and rendering.
    ClassicMap classicMap_;
    ///< Internal storage used to coordinate UI state and rendering.
    std::vector<CObstacle*> Obstacles;
    ///< Internal storage used to coordinate UI state and rendering.
    float obstacleSpawnTimer_ = 0.f;
    ///< Internal storage used to coordinate UI state and rendering.
    float cameraY_ = -1080.f;
    ///< Internal storage used to coordinate UI state and rendering.
    CPlayer player_;
    ///< Internal storage used to coordinate UI state and rendering.
    float elapsedPlaySec_ = 0.f;
    ///< Internal storage used to coordinate UI state and rendering.
    bool gameplayStarted_ = false;
    ///< Internal storage used to coordinate UI state and rendering.
    bool classicWon_ = false;

    ///< Internal storage used to coordinate UI state and rendering.
    enum class TrafficLight { Green, Yellow, Red };
    ///< Internal storage used to coordinate UI state and rendering.
    TrafficLight currentLight_ = TrafficLight::Green;
    ///< Internal storage used to coordinate UI state and rendering.
    float trafficLightTimer_ = 0.f;
    ///< Internal storage used to coordinate UI state and rendering.
    sf::Texture texTrafficGreen_, texTrafficYellow_, texTrafficRed_;
    ///< Internal storage used to coordinate UI state and rendering.
    sf::Texture texShieldItem_;
    ///< Internal storage used to coordinate UI state and rendering.
    sf::Sprite trafficLightSprite_;

    ///< Internal storage used to coordinate UI state and rendering.
    int  loadTabModeIdx_  = 0;
    ///< Internal storage used to coordinate UI state and rendering.
    int  selectedSlotIdx_ = -1;
    ///< Internal storage used to coordinate UI state and rendering.
    bool e_isMouseOverBack = false;

    ///< Internal storage used to coordinate UI state and rendering.
    SaveStore     saves_;
    ///< Internal storage used to coordinate UI state and rendering.
    RankingStore  ranks_;
    ///< Internal storage used to coordinate UI state and rendering.
    ProgressStore prog_;
    ///< Internal storage used to coordinate UI state and rendering.
    SettingsStore sets_;

    ///< Internal storage used to coordinate UI state and rendering.
    AudioManager audio_;
};
