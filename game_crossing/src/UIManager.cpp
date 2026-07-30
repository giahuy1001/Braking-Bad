/**
 * @file UIManager.cpp
 * @brief Implements UI rendering and interaction behavior.
 */
#include "UIManager.h"
#include "CCar.h"
#include "CTruck.h"
#include "CCat.h"
#include "CDeer.h"
#include "CPlayer.h"
#include "Grid.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <ctime>
#include <functional>
#include <iostream>
#include <limits>

namespace
{
    constexpr unsigned int  WINDOW_W   = 1920;
    constexpr unsigned int  WINDOW_H   = 1080;
    constexpr float         UI_SCALE   = 1.5f;
    constexpr float         UI_W       = WINDOW_W / UI_SCALE;
    constexpr float         UI_H       = WINDOW_H / UI_SCALE;
    constexpr float         LOADING_BLINK_PERIOD = 1.0f;
    constexpr float         LOADING_ZOOM_DURATION = 0.4f;
    constexpr float         LOADING_ZOOM_SCALE = 0.72f;
    constexpr float         LOADING_PROMPT_HEIGHT = 104.f;
    constexpr float         PADDING    = 24.f;
    constexpr float         BTN_W      = 360.f;
    constexpr float         BTN_H      = 56.f;
    constexpr float         BTN_GAP    = 16.f;
    constexpr float         BACK_SIZE  = 40.f;
    constexpr float         BACK_X     = UI_W - PADDING - BACK_SIZE;
    constexpr float         BACK_Y     = PADDING;
    constexpr int           NAME_MAX   = 16;
    constexpr int           CLASSIC_LEVELS = 10;
    constexpr float         ENDLESS_SCROLL_SPEED = 100.f;
    constexpr float         ENDLESS_CATCHUP_SPEED = 600.f;
    constexpr float         CLASSIC_FOLLOW_SPEED = 960.f;
    constexpr float         PLAYER_SCREEN_ANCHOR = Grid::MAP_HEIGHT * 0.65f;
    constexpr float         PLAYER_TOP_SAFE_LINE = 180.f;
    constexpr float         ENDLESS_CATCHUP_DISTANCE = Grid::CELL_SIZE * 5.f;

    sf::Color colorFromHex(unsigned int rgb)
    {
        return { static_cast<std::uint8_t>((rgb >> 16) & 0xFF),
                 static_cast<std::uint8_t>((rgb >>  8) & 0xFF),
                 static_cast<std::uint8_t>( rgb        & 0xFF) };
    }

    std::int64_t nowUnix()
    {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    std::string formatSaveTime(const RunRecord& record)
    {
        std::string result = std::to_string(record.elapsedSec) + "s";
        if (record.savedAtUnix <= 0)
            return result;

        const std::time_t timestamp = static_cast<std::time_t>(record.savedAtUnix);
        std::tm localTime{};
        if (localtime_s(&localTime, &timestamp) != 0)
            return result;

        char text[20]{};
        if (std::strftime(text, sizeof(text), "%Y-%m-%d %H:%M", &localTime) != 0)
            result += " / " + std::string(text);
        return result;
    }
}

const std::array<sf::FloatRect, 7> UIManager::kMainMenuButtonBounds = {

    sf::FloatRect({ 1416.f, 410.f }, { 314.f, 105.f }),
    sf::FloatRect({ 1416.f, 534.f }, { 314.f, 105.f }),
    sf::FloatRect({ 1416.f, 656.f }, { 314.f, 105.f }),
    sf::FloatRect({ 1416.f, 781.f }, { 314.f, 105.f }),

    sf::FloatRect({ 1555.f, 980.f }, { 94.f, 94.f }),
    sf::FloatRect({ 1684.f, 980.f }, { 94.f, 94.f }),
    sf::FloatRect({ 1807.f, 980.f }, { 94.f, 94.f })
};

const std::array<sf::FloatRect, 10> UIManager::kLevelButtonBounds = {
    sf::FloatRect({190.f, 245.f}, {125.f, 125.f}),
    sf::FloatRect({340.f, 245.f}, {125.f, 125.f}),
    sf::FloatRect({490.f, 245.f}, {125.f, 125.f}),
    sf::FloatRect({640.f, 245.f}, {125.f, 125.f}),
    sf::FloatRect({790.f, 245.f}, {125.f, 125.f}),
    sf::FloatRect({190.f, 500.f}, {125.f, 125.f}),
    sf::FloatRect({340.f, 500.f}, {125.f, 125.f}),
    sf::FloatRect({490.f, 500.f}, {125.f, 125.f}),
    sf::FloatRect({640.f, 500.f}, {125.f, 125.f}),
    sf::FloatRect({790.f, 500.f}, {125.f, 125.f})
};

const std::array<sf::FloatRect, 2> UIManager::kGameModeButtonBounds = {
    sf::FloatRect({ 500.f, 485.f }, { 385.f, 425.f }),
    sf::FloatRect({1000.f, 485.f }, { 385.f, 425.f })
};
const sf::FloatRect UIManager::kResultHomeButtonBounds({ 420.f, 552.f }, { 310.f, 115.f });


const std::array<std::string, 4> UIManager::kThemeNames = { "spring", "summer", "autumn", "winter" };
const sf::FloatRect UIManager::kCharacterPanelBounds({141.f, 380.f}, {399.f, 399.f});
const sf::FloatRect UIManager::kCharacterPrevBounds ({120.f, 554.f}, {48.f, 48.f});
const sf::FloatRect UIManager::kCharacterNextBounds ({513.f, 554.f}, {48.f, 48.f});
const sf::FloatRect UIManager::kThemePanelBounds    ({1374.f, 372.f}, {399.f, 399.f});
const sf::FloatRect UIManager::kThemePrevBounds     ({1353.f, 546.f}, {47.f, 47.f});
const sf::FloatRect UIManager::kThemeNextBounds     ({1746.f, 546.f}, {47.f, 47.f});
const sf::FloatRect UIManager::kSfxTrackBounds      ({1383.f, 484.f}, {400.f, 20.f});
const sf::FloatRect UIManager::kSfxDecBounds        ({1313.f, 469.f}, {50.f, 50.f});
const sf::FloatRect UIManager::kSfxIncBounds        ({1803.f, 469.f}, {50.f, 50.f});
const sf::FloatRect UIManager::kMusicTrackBounds    ({1383.f, 690.f}, {400.f, 20.f});
const sf::FloatRect UIManager::kMusicDecBounds      ({1313.f, 675.f}, {50.f, 50.f});
const sf::FloatRect UIManager::kMusicIncBounds      ({1803.f, 675.f}, {50.f, 50.f});
const sf::FloatRect UIManager::kSettingOkBounds     ({1504.f, 816.f}, {148.f, 102.f});
const sf::FloatRect UIManager::kGraphicOkThemeBounds ({1520.f, 801.f}, {155.f, 120.f});
const sf::FloatRect UIManager::kGraphicCharacterOkBounds ({288.f, 814.f}, {155.f, 120.f});

const std::array<sf::FloatRect, 6> kLoadSlotClickBounds = {
    sf::FloatRect({ 381.f, 416.f }, { 137.f, 115.f }),
    sf::FloatRect({ 381.f, 606.f }, { 137.f, 115.f }),
    sf::FloatRect({ 381.f, 800.f }, { 137.f, 115.f }),
    sf::FloatRect({1004.f, 416.f }, { 137.f, 115.f }),
    sf::FloatRect({1004.f, 606.f }, { 137.f, 115.f }),
    sf::FloatRect({1004.f, 800.f }, { 137.f, 115.f })
};

UIManager::UIManager(sf::RenderWindow& window)
    : win_(window),
    uiView_({ UI_W * 0.5f, UI_H * 0.5f }, { UI_W, UI_H }),
    debugText_(font_, "", 18),
    trafficLightSprite_(texTrafficGreen_)
{

    const std::array<std::string, 5> fontCandidates = {
        "assets/fonts/arial.ttf",
        "assets/font/arial.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/tahoma.ttf"
    };
    for (const std::string& path : fontCandidates)
    {
        if (font_.openFromFile(path))
        {
            fontLoaded_ = true;
            break;
        }
    }
    if (!fontLoaded_)
        std::cerr << "[UIManager] failed to load a UI/debug font\n";

    if (!rankingFont_.openFromFile("C:/Windows/Fonts/times.ttf"))
        std::cerr << "[UIManager] failed to load Times New Roman for ranking text\n";

    debugText_.setCharacterSize(18);
    debugText_.setFillColor(sf::Color::Yellow);
    debugText_.setOutlineColor(sf::Color::Black);
    debugText_.setOutlineThickness(2.f);

    const bool lg  = logoTex_.loadFromFile("../Graphic/1x/DataList.png");
    (void)        iconsTex_.loadFromFile("../Graphic/1x/DataList.png");
    (void)lg;

    bool tGreen = texTrafficGreen_.loadFromFile("assets/props/traffic_green.png");
    bool tYellow = texTrafficYellow_.loadFromFile("assets/props/traffic_yellow.png");
    bool tRed = texTrafficRed_.loadFromFile("assets/props/traffic_red.png");
    (void)tGreen; (void)tYellow; (void)tRed;

    bool tShield = texShieldItem_.loadFromFile("assets/props/shield.png");

    trafficLightSprite_.setTexture(texTrafficGreen_, true);

    sf::FloatRect bounds = trafficLightSprite_.getLocalBounds();
    trafficLightSprite_.setOrigin({ bounds.size.x / 2.f, 0.f });

    trafficLightSprite_.setPosition({ Grid::columnCenter(5), 0.f });

    cfg_ = sets_.load();
    cfg_.volume = std::clamp(cfg_.volume, 0, 100);
    cfg_.musicVolume = std::clamp(cfg_.musicVolume, 0, 100);
    applyAudioVolumes();
    cfg_.cosmetic.characterId = std::clamp(cfg_.cosmetic.characterId, 1,
                                           CharacterRenderer::kCharacterCount);
    cfg_.cosmetic.backgroundId = std::clamp(cfg_.cosmetic.backgroundId, 0,
                                             static_cast<int>(kThemeNames.size()) - 1);
    setTheme(kThemeNames.at(cfg_.cosmetic.backgroundId));
    ctx_.selectedCharacterID = cfg_.cosmetic.characterId;
    saves_.loadAll();
    ranks_.loadAll();
    prog_.load();

    if (!loadingTex_.loadFromFile("assets/LoadingScreen.png"))
        (void)loadingTex_.loadFromFile("assets/LoadingScreen.jpg");
    if (loadingTex_.getSize().x == 0 || loadingTex_.getSize().y == 0)
        std::cerr << "[UIManager] failed to load assets/LoadingScreen.png\n";

    loadingView_ = win_.getDefaultView();
    setState(UIState::Loading);

    audio_.loadAssets();
}

/**
 * @brief Releases transient resources after UI activity has ended.
 */
UIManager::~UIManager() = default;

/**
 * @brief Loads a complete seasonal asset set atomically so a failed asset never leaves a mixed theme on screen.
 */
bool UIManager::setTheme(const std::string& seasonName)
{
    if (std::find(kThemeNames.begin(), kThemeNames.end(), seasonName) == kThemeNames.end())
        return false;

    std::string folder = seasonName;
    folder[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(folder[0])));

    auto load = [&](sf::Texture& texture, const std::string& image, bool allowJpg) {
        const std::array<std::string, 2> folders = { seasonName, folder };
        for (const std::string& candidateFolder : folders)
        {
            const std::string stem = "assets/theme/" + candidateFolder + "/" + image;
            if (texture.loadFromFile(stem + ".png")) return true;
            if (allowJpg && texture.loadFromFile(stem + ".jpg")) return true;
            if (allowJpg && texture.loadFromFile(stem + ".jpeg")) return true;
        }
        return false;
    };

    sf::Texture newMain, newSetting, newGraphic, newLoad, newRanking, newBackButton, newSettingButton, newPause;
    sf::Texture newScoreTable, newSave, newQuit, newLevel, newUserName, newGameMode, newWin, newLose, newIconTheme;

    const bool loadScreenLoaded = load(newLoad, "Load", false);
    if (!loadScreenLoaded)
        std::cerr << "[UIManager] optional Load.png is missing for theme '"
                  << seasonName << "'; using the normal seasonal background.\n";

    const bool rankingLoaded = load(newRanking, "Ranking", false);
    if (!load(newMain, "MainMenu", true) || !load(newSetting, "Setting", false) ||
        !load(newGraphic, "Graphic", false) || !rankingLoaded ||
        !load(newBackButton, "BackButton", false) ||
        !load(newSettingButton, "SettingButton", false))
    {
        std::cerr << "[UIManager] failed to load theme '" << seasonName
                  << "' from assets/theme/<season>/\n";
        return false;
    }

    bgTex_ = std::move(newMain);
    settingBgTex_ = std::move(newSetting);
    graphicBgTex_ = std::move(newGraphic);

    loadBgTex_ = std::move(newLoad);
    rankingBgTex_ = std::move(newRanking);
    backButtonTex_ = std::move(newBackButton);
    settingButtonTex_ = std::move(newSettingButton);
    // The active season's icon is shared by Graphic preview and Level cards.
    if (load(newIconTheme, "IconTheme", false))
        iconThemeTex_ = std::move(newIconTheme);
    else
        std::cerr << "[UIManager] missing IconTheme.png for theme '" << seasonName << "'\n";
    levelBgAssetLoaded_ = load(newLevel, "Level", false);
    if (levelBgAssetLoaded_)
        levelBgTex_ = std::move(newLevel);
    else
        std::cerr << "[UIManager] missing Level.png for theme '" << seasonName << "'\n";

    userNameBgAssetLoaded_ = load(newUserName, "UserName", false);
    if (userNameBgAssetLoaded_)
        userNameBgTex_ = std::move(newUserName);
    else
        std::cerr << "[UIManager] missing UserName.png for theme '" << seasonName << "'\n";

    gameModeBgAssetLoaded_ = load(newGameMode, "GameMode", false);
    if (gameModeBgAssetLoaded_)
        gameModeBgTex_ = std::move(newGameMode);
    else
        std::cerr << "[UIManager] missing GameMode.png for theme '" << seasonName << "'\n";

    winAssetLoaded_ = load(newWin, "Win", false);
    if (winAssetLoaded_)
        winTex_ = std::move(newWin);
    else
        std::cerr << "[UIManager] missing Win.png for theme '" << seasonName << "'\n";

    loseAssetLoaded_ = load(newLose, "Lose", false);
    if (loseAssetLoaded_)
        loseTex_ = std::move(newLose);
    else
        std::cerr << "[UIManager] missing Lose.png for theme '" << seasonName << "'\n";

    scoreTableAssetLoaded_ = load(newScoreTable, "ScoreTable", false);
    if (scoreTableAssetLoaded_)
        scoreTableTex_ = std::move(newScoreTable);
    else
        std::cerr << "[UIManager] missing ScoreTable.png for theme '" << seasonName << "'\n";

    saveAssetLoaded_ = load(newSave, "Save", false);
    if (saveAssetLoaded_)
        saveTex_ = std::move(newSave);
    else
        std::cerr << "[UIManager] optional Save.png is missing for theme '" << seasonName << "'\n";

    quitAssetLoaded_ = load(newQuit, "Quit", false);
    if (quitAssetLoaded_)
        quitTex_ = std::move(newQuit);
    else
        std::cerr << "[UIManager] optional Quit.png is missing for theme '" << seasonName << "'\n";
    pauseAssetLoaded_ = load(newPause, "Pause", false);
    if (pauseAssetLoaded_)
        pauseTex_ = std::move(newPause);
    else
        std::cerr << "[UIManager] optional Pause.png is missing for theme '"
                  << seasonName << "'; using the normal seasonal background.\n";
    currentTheme_ = seasonName;
    currentThemeIndex_ = static_cast<int>(std::distance(kThemeNames.begin(),
        std::find(kThemeNames.begin(), kThemeNames.end(), currentTheme_)));
    if (!mapBackground_.loadTheme(currentTheme_))
        std::cerr << "[UIManager] no gameplay maps found for theme '" << currentTheme_ << "'\n";
    endlessMap_.setAvailableMapLevels(mapBackground_.availableLevelNumbers());
    assetsLoaded_ = true;
    return true;
}

/**
 * @brief Performs the load graphic preview theme icon operation while preserving the current UI state invariants.
 */
bool UIManager::loadGraphicPreviewThemeIcon()
{
    const std::string& season = kThemeNames.at(previewThemeIndex_);
    std::string legacyFolder = season;
    legacyFolder[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(legacyFolder[0])));

    sf::Texture loaded;
    for (const std::string& folder : { season, legacyFolder }) {
        if (loaded.loadFromFile("assets/theme/" + folder + "/IconTheme.png")) {
            iconThemeTex_ = std::move(loaded);
            return true;
        }
    }
    std::cerr << "[UIManager] missing IconTheme.png for preview theme '" << season << "'\n";
    return false;
}

/**
 * @brief Performs the begin graphic preview operation while preserving the current UI state invariants.
 */
void UIManager::beginGraphicPreview()
{
    previewThemeIndex_ = currentThemeIndex_;
    previewCharacterId_ = CharacterRenderer::normalizeID(cfg_.cosmetic.characterId);
    loadGraphicPreviewThemeIcon();
}

/**
 * @brief Performs the commit graphic preview operation while preserving the current UI state invariants.
 */
bool UIManager::commitGraphicPreview()
{
    const bool themeApplied = commitPreviewTheme();
    commitPreviewCharacter();
    return themeApplied;
}

/**
 * @brief Performs the commit preview theme operation while preserving the current UI state invariants.
 */
bool UIManager::commitPreviewTheme()
{

    if (!setTheme(kThemeNames.at(previewThemeIndex_)))
        return false;

    cfg_.cosmetic.backgroundId = previewThemeIndex_;
    sets_.save(cfg_);
    return true;
}

/**
 * @brief Performs the commit preview character operation while preserving the current UI state invariants.
 */
void UIManager::commitPreviewCharacter()
{
    cfg_.cosmetic.characterId = previewCharacterId_;
    ctx_.selectedCharacterID = previewCharacterId_;
    sets_.save(cfg_);
}

/**
 * @brief Executes one complete UI lifecycle until the application closes.
 */
void UIManager::run()
{
    while (win_.isOpen())
    {
        handleEvents();
        update(clock_.restart().asSeconds());
        render();
    }
}

/**
 * @brief Routes queued input to exactly one state handler so modal input cannot leak into the underlying screen.
 */
void UIManager::handleEvents()
{
    while (const std::optional<sf::Event> event = win_.pollEvent())
    {
        if (event->is<sf::Event::Closed>())

        {
            win_.close();
            return;
        }

        if (const auto* key = event->getIf<sf::Event::KeyPressed>())
        {
            if (key->code == sf::Keyboard::Key::F8)
            {
                debugAudioMixer_ = !debugAudioMixer_;
                std::cout << "[Audio Mixer] Debug mixer "
                          << (debugAudioMixer_ ? "enabled" : "disabled") << '\n';
                continue;
            }

            if (key->code == sf::Keyboard::Key::F9) {
                audio_.playUiClick();
                audio_.cycleBgm(1);
                return;
            }

            if (debugAudioMixer_ && handleDebugAudioMixerKey(*key))
                continue;

            if (key->code == sf::Keyboard::Key::F7)
            {
                debugUi_ = !debugUi_;
                std::cout << "[UI Debug] " << (debugUi_ ? "enabled" : "disabled") << '\n';
                continue;
            }
        }

        if (debugUi_)
        {
            if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>();
                mouse && mouse->button == sf::Mouse::Button::Left)
            {
                const sf::Vector2f base = toBaseCoords(mouse->position);
                std::cout << "[UI Debug] Clicked at Screen(" << mouse->position.x << ", "
                          << mouse->position.y << ") | Base("
                          << static_cast<int>(std::lround(base.x)) << ", "
                          << static_cast<int>(std::lround(base.y)) << ")\n";
                continue;
            }
        }

        // Modals are rendered last because they must intercept attention and input from the underlying screen.
    if (modal_ != Modal::None)
        {
            handleModal(*event);
            continue;
        }

        switch (state_)
        {
        case UIState::Loading:      handleLoading(*event);  break;
        case UIState::MainMenu:     handleMainMenu(*event); break;
        case UIState::ModeSelect:   handleModeSel(*event);  break;
        case UIState::NameInput:    handleName(*event);     break;
        case UIState::LevelSelect:  handleLvlSel(*event);   break;
        case UIState::Setting:      handleSetting(*event);  break;
        case UIState::Graphic:      handleGraphic(*event);  break;
        case UIState::LoadGame:     handleLoad(*event);     break;
        case UIState::Ranking:      handleRanking(*event);  break;
        case UIState::Help:         handleHelp(*event);     break;
        case UIState::ClassicPlay:
        case UIState::EndlessPlay:  handlePlay(*event);     break;
        case UIState::GameOver:     handleGameOver(*event); break;
        case UIState::Pause:        handlePause(*event);    break;
        }
    }
}

/**
 * @brief Performs the handle back operation while preserving the current UI state invariants.
 */
void UIManager::handleBack()
{
    switch (state_)
    {
    case UIState::ModeSelect:   setState(UIState::MainMenu); break;
    case UIState::NameInput:    setState(UIState::ModeSelect); break;
    case UIState::LevelSelect:  setState(UIState::NameInput);  break;
    case UIState::Setting:
    case UIState::Graphic:
    case UIState::LoadGame:
    case UIState::Ranking:
    case UIState::Help:         setState(UIState::MainMenu); break;
    case UIState::Pause:        setState(UIState::MainMenu); break;
    case UIState::GameOver:     setState(UIState::MainMenu); break;
    case UIState::ClassicPlay:
    case UIState::EndlessPlay:  setState(UIState::Pause);    break;
    default: break;
    }
}

/**
 * @brief Handles startup confirmation and starts the loading-screen zoom transition.
 */
void UIManager::handleLoading(const sf::Event& e)
{
    const auto* key = e.getIf<sf::Event::KeyPressed>();
    const auto* mouse = e.getIf<sf::Event::MouseButtonPressed>();
    const bool enterPressed = key && key->code == sf::Keyboard::Key::Enter;
    const bool leftClicked = mouse && mouse->button == sf::Mouse::Button::Left;
    if (!loadingTransitionActive_ && (enterPressed || leftClicked)) {
        audio_.playUiClick();
        loadingTransitionActive_ = true;
        loadingZoomElapsed_ = 0.f;
    }
}

/**
 * @brief Performs the handle main menu operation while preserving the current UI state invariants.
 */
void UIManager::handleMainMenu(const sf::Event& e)
{
    if (const auto* key = e.getIf<sf::Event::KeyPressed>())
    {
        if (key->code == sf::Keyboard::Key::Escape) { modal_ = Modal::ConfirmExit; return; }
        if (key->code == sf::Keyboard::Key::Up)   { focusIdx_ = (focusIdx_ + 6) % 7; return; }
        if (key->code == sf::Keyboard::Key::Down) { focusIdx_ = (focusIdx_ + 1) % 7; return; }
        if (key->code == sf::Keyboard::Key::Enter) { activateMainMenuButton(focusIdx_); return; }
    }
    if (const auto* mm = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mm->button == sf::Mouse::Button::Left)
        {
            const int button = menuButtonAt(mm->position);
            if (button >= 0)
            {
                audio_.playUiClick();
                activateMainMenuButton(static_cast<std::size_t>(button));
            }
        }
    }
    if (e.is<sf::Event::MouseMoved>())
    {
        const auto* mm = e.getIf<sf::Event::MouseMoved>();
        const int button = menuButtonAt(mm->position);

        if (button >= 0 && button != focusIdx_) {
            audio_.playUiHover();
            focusIdx_ = button;
        }
    }
}

/**
 * @brief Performs the handle mode sel operation while preserving the current UI state invariants.
 */
void UIManager::handleModeSel(const sf::Event& e)
{
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape || k->code == sf::Keyboard::Key::Backspace) { handleBack(); return; }
        if (k->code == sf::Keyboard::Key::Left || k->code == sf::Keyboard::Key::Right) {
            focusIdx_ = 1 - focusIdx_;
            audio_.playUiHover();
            return;
        }
        if (k->code == sf::Keyboard::Key::Enter) {
            selectGameMode(focusIdx_ == 0 ? StateContext::Mode::Classic : StateContext::Mode::Endless);
            return;
        }
    }
    if (const auto* mm = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mm->button == sf::Mouse::Button::Left)
        {
            const sf::Vector2f mouse(static_cast<float>(mm->position.x),
                                     static_cast<float>(mm->position.y));
            for (std::size_t i = 0; i < kGameModeButtonBounds.size(); ++i)
                if (gameModeButtonBounds(i).contains(mouse)) {
                    focusIdx_ = static_cast<int>(i);
                    selectGameMode(i == 0 ? StateContext::Mode::Classic : StateContext::Mode::Endless);
                    return;
                }
        }
    }
    if (e.is<sf::Event::MouseMoved>())
    {
        const auto* mm = e.getIf<sf::Event::MouseMoved>();
        const sf::Vector2f mouse(static_cast<float>(mm->position.x),
                                 static_cast<float>(mm->position.y));
        for (std::size_t i = 0; i < kGameModeButtonBounds.size(); ++i)
            if (gameModeButtonBounds(i).contains(mouse) && focusIdx_ != static_cast<int>(i)) {
                focusIdx_ = static_cast<int>(i);
                audio_.playUiHover();
            }
    }
}

/**
 * @brief Performs the select game mode operation while preserving the current UI state invariants.
 */
void UIManager::selectGameMode(StateContext::Mode mode)
{
    audio_.playUiClick();
    ctx_.mode = mode;
    ctx_.selectedCharacterID = cfg_.cosmetic.characterId;
    nameBuffer_.clear();
    setState(UIState::NameInput);
}

/**
 * @brief Performs the game mode screen bounds operation while preserving the current UI state invariants.
 */
sf::FloatRect UIManager::gameModeScreenBounds() const
{
    const sf::Vector2u size = gameModeBgTex_.getSize();
    const sf::Vector2u windowSize = win_.getSize();
    return { { (windowSize.x - static_cast<float>(size.x)) * .5f,
               (windowSize.y - static_cast<float>(size.y)) * .5f },
             { static_cast<float>(size.x), static_cast<float>(size.y) } };
}

/**
 * @brief Performs the game mode button bounds operation while preserving the current UI state invariants.
 */
sf::FloatRect UIManager::gameModeButtonBounds(std::size_t index) const
{
    const sf::FloatRect local = kGameModeButtonBounds.at(index);
    return { gameModeScreenBounds().position + local.position, local.size };
}

/**
 * @brief Performs the handle name operation while preserving the current UI state invariants.
 */
void UIManager::handleName(const sf::Event& e)
{
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape) { audio_.playUiClick(); handleBack(); return; }
        if (k->code == sf::Keyboard::Key::Backspace) { audio_.playUiClick(); if (!nameBuffer_.empty()) nameBuffer_.pop_back(); return; }
        if (k->code == sf::Keyboard::Key::Enter) { confirmName(); return; }
    }
    if (const auto* t = e.getIf<sf::Event::TextEntered>())
    {
        if (isValidNameChar(static_cast<std::uint32_t>(t->unicode)) && (int)nameBuffer_.size() < NAME_MAX)
        {
            audio_.playUiClick();
            nameBuffer_ += static_cast<char>(t->unicode);
        }
    }
    if (const auto* mm = e.getIf<sf::Event::MouseButtonPressed>();
        mm && mm->button == sf::Mouse::Button::Left && userNameConfirmBounds().contains(
            { static_cast<float>(mm->position.x), static_cast<float>(mm->position.y) }))
        confirmName();
}

/**
 * @brief Performs the confirm name operation while preserving the current UI state invariants.
 */
void UIManager::confirmName()
{
    if (!isValidName(nameBuffer_)) return;
    audio_.playUiClick();
    ctx_.pendingName = nameBuffer_;
    if (ctx_.mode == StateContext::Mode::Classic) setState(UIState::LevelSelect);
    else setState(UIState::EndlessPlay);
}

/**
 * @brief Performs the handle lvl sel operation while preserving the current UI state invariants.
 */
void UIManager::handleLvlSel(const sf::Event& e)
{
    auto selectFocusedLevel = [this] {
        startClassicLevel(focusIdx_ + 1);
    };

    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape || k->code == sf::Keyboard::Key::Backspace) { audio_.playUiClick(); handleBack(); return; }
        if (k->code == sf::Keyboard::Key::Enter) { audio_.playUiClick(); selectFocusedLevel(); return; }
        if (k->code == sf::Keyboard::Key::Left) { audio_.playUiHover(); focusIdx_ = (focusIdx_ + 9) % 10; return; }
        if (k->code == sf::Keyboard::Key::Right) { audio_.playUiHover(); focusIdx_ = (focusIdx_ + 1) % 10; return; }
        if (k->code == sf::Keyboard::Key::Up) { audio_.playUiHover(); focusIdx_ = (focusIdx_ + 5) % 10; return; }
        if (k->code == sf::Keyboard::Key::Down) { audio_.playUiHover(); focusIdx_ = (focusIdx_ + 5) % 10; return; }
    }
    if (const auto* mm = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mm->button == sf::Mouse::Button::Left)
        {
            sf::Vector2f mp(static_cast<float>(mm->position.x), static_cast<float>(mm->position.y));
            for (int i = 0; i < CLASSIC_LEVELS; ++i)
            {
                if (levelButtonBounds(i).contains(mp))
                {
                    audio_.playUiClick();
                    focusIdx_ = i;
                    startClassicLevel(i + 1);
                    return;
                }
            }
        }
    }
    if (e.is<sf::Event::MouseMoved>())
    {
        const auto* mm = e.getIf<sf::Event::MouseMoved>();
        sf::Vector2f mp(static_cast<float>(mm->position.x), static_cast<float>(mm->position.y));
        for (int i = 0; i < CLASSIC_LEVELS; ++i)
        {
            if (levelButtonBounds(i).contains(mp)) {
                if (focusIdx_ != i) audio_.playUiHover();
                focusIdx_ = i;
            }
        }
    }
}

/**
 * @brief Performs the start classic level operation while preserving the current UI state invariants.
 */
void UIManager::startClassicLevel(int level)
{
    ctx_.level = std::clamp(level, 1, CLASSIC_LEVELS);
    setState(UIState::ClassicPlay);
}

/**
 * @brief Performs the level screen bounds operation while preserving the current UI state invariants.
 */
sf::FloatRect UIManager::levelScreenBounds() const
{
    const sf::Vector2u size = levelBgTex_.getSize();
    const sf::Vector2u windowSize = win_.getSize();
    return { { (windowSize.x - static_cast<float>(size.x)) * .5f,
               (windowSize.y - static_cast<float>(size.y)) * .5f },
             { static_cast<float>(size.x), static_cast<float>(size.y) } };
}

/**
 * @brief Performs the user name screen bounds operation while preserving the current UI state invariants.
 */
sf::FloatRect UIManager::userNameScreenBounds() const
{
    const sf::Vector2u size = userNameBgTex_.getSize();
    const sf::Vector2u windowSize = win_.getSize();
    return { { (windowSize.x - static_cast<float>(size.x)) * .5f,
               (windowSize.y - static_cast<float>(size.y)) * .5f },
             { static_cast<float>(size.x), static_cast<float>(size.y) } };
}

/**
 * @brief Performs the level button bounds operation while preserving the current UI state invariants.
 */
sf::FloatRect UIManager::levelButtonBounds(std::size_t index) const
{
    const sf::FloatRect local = kLevelButtonBounds.at(index);
    const sf::FloatRect screen = levelScreenBounds();
    return { screen.position + local.position, local.size };
}

/**
 * @brief Performs the user name input bounds operation while preserving the current UI state invariants.
 */
sf::FloatRect UIManager::userNameInputBounds() const
{
    const sf::FloatRect screen = userNameScreenBounds();
    return { screen.position + sf::Vector2f(220.f, 335.f), { 697.f, 82.f } };
}

/**
 * @brief Performs the user name confirm bounds operation while preserving the current UI state invariants.
 */
sf::FloatRect UIManager::userNameConfirmBounds() const
{
    const sf::FloatRect screen = userNameScreenBounds();
    return { screen.position + sf::Vector2f(420.f, 500.f), { 297.f, 76.f } };
}

/**
 * @brief Performs the handle setting operation while preserving the current UI state invariants.
 */
void UIManager::handleSetting(const sf::Event& e)
{
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape) { handleBack(); return; }
        if (k->code == sf::Keyboard::Key::Left) { cfg_.volume = std::max(0, cfg_.volume - 5); audio_.playUiClick(); applyAudioVolumes(); return; }
        if (k->code == sf::Keyboard::Key::Right) { cfg_.volume = std::min(100, cfg_.volume + 5); audio_.playUiClick(); applyAudioVolumes(); return; }
    }
    if (const auto* mm = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mm->button == sf::Mouse::Button::Left)
        {
            const sf::Vector2f p(mm->position.x, mm->position.y);
            if (scaledBaseRect(kSettingOkBounds).contains(p)) { audio_.playUiClick(); sets_.save(cfg_); setState(UIState::MainMenu); return; }
            if (scaledBaseRect(kSfxDecBounds).contains(p)) { audio_.playUiClick(); cfg_.volume = std::max(0, cfg_.volume - 5); applyAudioVolumes(); return; }
            if (scaledBaseRect(kSfxIncBounds).contains(p)) { audio_.playUiClick(); cfg_.volume = std::min(100, cfg_.volume + 5); applyAudioVolumes(); return; }
            if (scaledBaseRect(kMusicDecBounds).contains(p)) { audio_.playUiClick(); cfg_.musicVolume = std::max(0, cfg_.musicVolume - 5); applyAudioVolumes(); return; }
            if (scaledBaseRect(kMusicIncBounds).contains(p)) { audio_.playUiClick(); cfg_.musicVolume = std::min(100, cfg_.musicVolume + 5); applyAudioVolumes(); return; }
            if (scaledBaseRect(kSfxTrackBounds).contains(p)) { draggingSfx_ = true; setSfxVolumeFromMouse(mm->position); return; }
            if (scaledBaseRect(kMusicTrackBounds).contains(p)) { draggingMusic_ = true; setMusicVolumeFromMouse(mm->position); return; }
        }
    }

    if (e.is<sf::Event::MouseMoved>())
    {
        const auto* mm = e.getIf<sf::Event::MouseMoved>();
        if (draggingSfx_) setSfxVolumeFromMouse(mm->position);
        if (draggingMusic_) setMusicVolumeFromMouse(mm->position);
    }
    if (e.is<sf::Event::MouseButtonReleased>()) { draggingSfx_ = draggingMusic_ = false; }
}

/**
 * @brief Performs the handle graphic operation while preserving the current UI state invariants.
 */
void UIManager::handleGraphic(const sf::Event& e)
{
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape) { audio_.playUiClick(); handleBack(); return; }
        if (k->code == sf::Keyboard::Key::Left)
        {
            audio_.playUiClick();
            previewCharacterId_ = CharacterRenderer::normalizeID(previewCharacterId_ - 1);
            return;
        }
        if (k->code == sf::Keyboard::Key::Right)
        {
            audio_.playUiClick();
            previewCharacterId_ = CharacterRenderer::normalizeID(previewCharacterId_ + 1);
            return;
        }
    }
    if (const auto* mm = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mm->button == sf::Mouse::Button::Left)
        {
            const sf::Vector2f p(mm->position.x, mm->position.y);
            if (scaledBaseRect(kCharacterPrevBounds).contains(p)) { audio_.playUiClick(); previewCharacterId_ = CharacterRenderer::normalizeID(previewCharacterId_ - 1); return; }
            if (scaledBaseRect(kCharacterNextBounds).contains(p)) { audio_.playUiClick(); previewCharacterId_ = CharacterRenderer::normalizeID(previewCharacterId_ + 1); return; }
            if (scaledBaseRect(kThemePrevBounds).contains(p)) { audio_.playUiClick(); previewThemeIndex_ = (previewThemeIndex_ + 3) % 4; loadGraphicPreviewThemeIcon(); return; }
            if (scaledBaseRect(kThemeNextBounds).contains(p)) { audio_.playUiClick(); previewThemeIndex_ = (previewThemeIndex_ + 1) % 4; loadGraphicPreviewThemeIcon(); return; }
            if (scaledBaseRect(kGraphicCharacterOkBounds).contains(p)) { audio_.playUiClick(); commitPreviewCharacter(); return; }
            if (scaledBaseRect(kGraphicOkThemeBounds).contains(p)) { audio_.playUiClick(); commitPreviewTheme(); return; }
        }
    }
}

/**
 * @brief Performs the handle load operation while preserving the current UI state invariants.
 */
void UIManager::handleLoad(const sf::Event& e)
{
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape || k->code == sf::Keyboard::Key::Backspace) { audio_.playUiClick(); handleBack(); return; }
    }
    if (const auto* mm = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mm->button == sf::Mouse::Button::Left)
        {
            const sf::Vector2f point = toBaseCoords(mm->position);
            for (int column = 0; column < 2; ++column) {
                const GameMode mode = column == 0 ? GameMode::Classic : GameMode::Endless;
                const auto slots = saves_.slots(mode);
                for (int place = 0; place < SaveStore::kMaxSlots; ++place) {
                    const sf::FloatRect& bounds = kLoadSlotClickBounds[column * SaveStore::kMaxSlots + place];
                    if (!bounds.contains(point) || slots[place].name.empty())
                        continue;

                    const RunRecord& save = slots[place];
                    audio_.playUiClick();
                    ctx_.pendingName = save.name;
                    ctx_.level = save.level;
                    ctx_.classicLevel = save.level;
                    ctx_.classicSec = save.elapsedSec;
                    ctx_.mode = mode == GameMode::Endless ? StateContext::Mode::Endless : StateContext::Mode::Classic;
                    setState(mode == GameMode::Endless ? UIState::EndlessPlay : UIState::ClassicPlay);
                    player_.setPosition({ save.playerX, save.playerY });
                    player_.revive();
                    cameraY_ = save.cameraY;
                    player_.setCameraOffset(cameraY_);
                    elapsedPlaySec_ = static_cast<float>(save.elapsedSec);
                    return;
                }
            }
        }
    }
}

/**
 * @brief Performs the handle ranking operation while preserving the current UI state invariants.
 */
void UIManager::handleRanking(const sf::Event& e)
{
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape) { audio_.playUiClick(); handleBack(); return; }
    }
}

/**
 * @brief Performs the handle help operation while preserving the current UI state invariants.
 */
void UIManager::handleHelp(const sf::Event& e)
{
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape) {
            audio_.playUiClick();
            handleBack();
        }
    }
}

/**
 * @brief Performs the handle play operation while preserving the current UI state invariants.
 */
void UIManager::handlePlay(const sf::Event& e)
{
    if (const auto* mm = e.getIf<sf::Event::MouseButtonPressed>();
        mm && mm->button == sf::Mouse::Button::Left)
    {
        const sf::Vector2f mouse(static_cast<float>(mm->position.x),
                                 static_cast<float>(mm->position.y));
        if (sidebarPauseBounds().contains(mouse)) {
            audio_.playUiClick();
            setState(UIState::Pause);
            return;
        }
        if (sidebarSaveBounds().contains(mouse)) {
            audio_.playUiClick();
            modalFocus_ = 0;
            modal_ = Modal::SaveGame;
            return;
        }
        if (sidebarQuitBounds().contains(mouse)) {
            audio_.playUiClick();
            modalFocus_ = 0;
            modal_ = Modal::QuitGame;
            return;
        }
    }

    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape) { handleBack(); return; }

        const bool isMove = k->code == sf::Keyboard::Key::Left ||
            k->code == sf::Keyboard::Key::Right ||
            k->code == sf::Keyboard::Key::Up ||
            k->code == sf::Keyboard::Key::Down ||
            k->code == sf::Keyboard::Key::A ||
            k->code == sf::Keyboard::Key::D ||
            k->code == sf::Keyboard::Key::W ||
            k->code == sf::Keyboard::Key::S;
        if (!isMove) return;

        if (state_ == UIState::EndlessPlay && isEndlessPlayerOffscreen())
        {
            finishEndlessRun();
            return;
        }

        gameplayStarted_ = true;

        const float topLimit = state_ == UIState::ClassicPlay
            ? classicMap_.topLimit() + Grid::CELL_SIZE * 0.5f
            : std::numeric_limits<float>::lowest() / 4.f;
        const float mapBottom = state_ == UIState::ClassicPlay
            ? classicMap_.bottomLimit() - Grid::CELL_SIZE * 0.5f
            : -Grid::CELL_SIZE * 0.5f;

        const float minY = topLimit;
        const float maxY = std::min(mapBottom, maxWalkablePlayerY());

        player_.setMovementBounds({ { Grid::playableLeftCenter(), minY },
                                    { Grid::playableRightCenter() - Grid::playableLeftCenter(),
                                      maxY - minY } });
        player_.handleInput(k->code);
    }
}

/**
 * @brief Performs the handle game over operation while preserving the current UI state invariants.
 */
void UIManager::handleGameOver(const sf::Event& e)
{
    if (const auto* mouse = e.getIf<sf::Event::MouseButtonPressed>();
        mouse && mouse->button == sf::Mouse::Button::Left && resultHomeBounds().contains(
            { static_cast<float>(mouse->position.x), static_cast<float>(mouse->position.y) })) {
        returnFromGameOver();
        return;
    }
    if (const auto* key = e.getIf<sf::Event::KeyPressed>();
        key && (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Escape))
        returnFromGameOver();
}

sf::FloatRect UIManager::resultScreenBounds() const
{
    const sf::Texture& texture = classicWon_ ? winTex_ : loseTex_;
    const sf::Vector2u size = texture.getSize();
    const sf::Vector2u windowSize = win_.getSize();
    return { { (windowSize.x - static_cast<float>(size.x)) * .5f,
               (windowSize.y - static_cast<float>(size.y)) * .5f },
             { static_cast<float>(size.x), static_cast<float>(size.y) } };
}

sf::FloatRect UIManager::resultHomeBounds() const
{
    const sf::FloatRect screen = resultScreenBounds();
    return { screen.position + kResultHomeButtonBounds.position, kResultHomeButtonBounds.size };
}

void UIManager::returnFromGameOver()
{
    RunRecord record;
    record.name = ctx_.pendingName;
    record.mode = ctx_.mode == StateContext::Mode::Classic ? GameMode::Classic : GameMode::Endless;
    record.level = record.mode == GameMode::Classic ? ctx_.classicLevel : 0;
    record.elapsedSec = record.mode == GameMode::Classic ? ctx_.classicSec : ctx_.endlessSec;
    record.score = record.mode == GameMode::Classic ? 0 : ctx_.endlessScore;
    record.savedAtUnix = nowUnix();
    ranks_.submit(record);
    audio_.playUiClick();
    classicWon_ = false;
    setState(UIState::MainMenu);
}

/**
 * @brief Performs the handle pause operation while preserving the current UI state invariants.
 */
void UIManager::handlePause(const sf::Event& e)
{
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape)
        {
            audio_.playUiClick();
            savePausedRunAndExit();
            return;
        }
        if (k->code == sf::Keyboard::Key::Enter)
        {
            audio_.playUiClick();
            setState(ctx_.mode == StateContext::Mode::Endless
                ? UIState::EndlessPlay
                : UIState::ClassicPlay);
        }
    }
    if (const auto* mm = e.getIf<sf::Event::MouseButtonPressed>();
        mm && mm->button == sf::Mouse::Button::Left)
    {
        const sf::Vector2f mouse(static_cast<float>(mm->position.x), static_cast<float>(mm->position.y));
        if (pauseResumeBounds().contains(mouse)) {
            audio_.playUiClick();
            setState(ctx_.mode == StateContext::Mode::Endless ? UIState::EndlessPlay : UIState::ClassicPlay);
            return;
        }
        if (pauseOutBounds().contains(mouse)) {
            audio_.playUiClick();
            savePausedRunAndExit();
            return;
        }
    }
}

/**
 * @brief Performs the save paused run and exit operation while preserving the current UI state invariants.
 */
void UIManager::savePausedRunAndExit()
{
    saveCurrentRun();
    setState(UIState::MainMenu);
}

/**
 * @brief Performs the save current run operation while preserving the current UI state invariants.
 */
void UIManager::saveCurrentRun()
{
    RunRecord record;
    record.name = ctx_.pendingName;
    record.mode = ctx_.mode == StateContext::Mode::Classic ? GameMode::Classic : GameMode::Endless;
    record.level = record.mode == GameMode::Classic ? ctx_.level : 0;
    record.elapsedSec = static_cast<int>(elapsedPlaySec_);
    record.score = record.mode == GameMode::Classic ? 0 : ctx_.endlessScore;
    record.savedAtUnix = nowUnix();
    record.playerX = player_.getPosition().x;
    record.playerY = player_.getPosition().y;
    record.cameraY = cameraY_;
    saves_.push(record);
}

/**
 * @brief Performs the pause overlay bounds operation while preserving the current UI state invariants.
 */
sf::FloatRect UIManager::pauseOverlayBounds() const
{
    constexpr sf::Vector2f overlaySize(1200.f, 870.f);
    const sf::Vector2u windowSize = win_.getSize();
    const sf::Vector2f windowSizeF(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y));
    return { { (windowSizeF.x - overlaySize.x) * .5f, (windowSizeF.y - overlaySize.y) * .5f }, overlaySize };
}

/**
 * @brief Performs the pause resume bounds operation while preserving the current UI state invariants.
 */
sf::FloatRect UIManager::pauseResumeBounds() const
{
    const sf::FloatRect overlay = pauseOverlayBounds();

    return { { overlay.position.x + 372.f, overlay.position.y + 307.f }, { 477.f, 205.f } };
}

/**
 * @brief Performs the pause out bounds operation while preserving the current UI state invariants.
 */
sf::FloatRect UIManager::pauseOutBounds() const
{
    const sf::FloatRect overlay = pauseOverlayBounds();
    return { { overlay.position.x + 372.f, overlay.position.y + 560.f }, { 477.f, 205.f } };
}

/**
 * @brief Performs the capture paused frame operation while preserving the current UI state invariants.
 */
void UIManager::capturePausedFrame()
{
    const sf::Vector2u windowSize = win_.getSize();
    if (windowSize.x == 0 || windowSize.y == 0) { pauseFrameValid_ = false; return; }
    if (pauseFrameTex_.getSize() != windowSize && !pauseFrameTex_.resize(windowSize)) {
        pauseFrameValid_ = false;
        return;
    }
    pauseFrameTex_.update(win_);
    pauseFrameValid_ = true;
}

/**
 * @brief Consumes dialog input before state input to prevent an action from being applied twice.
 */
void UIManager::handleModal(const sf::Event& e)
{
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape) { audio_.playUiClick(); modal_ = Modal::None; return; }
        if (k->code == sf::Keyboard::Key::Left || k->code == sf::Keyboard::Key::Right ||
            k->code == sf::Keyboard::Key::Up || k->code == sf::Keyboard::Key::Down) {
            audio_.playUiHover();
            modalFocus_ = 1 - modalFocus_;
            return;
        }
        if (k->code == sf::Keyboard::Key::Enter)
        {
            audio_.playUiClick();
            if (modalFocus_ != 0) { modal_ = Modal::None; return; }
            if (modal_ == Modal::ConfirmExit) win_.close();
            else if (modal_ == Modal::SaveGame) {
                saveCurrentRun();
                modal_ = Modal::None;
                setState(UIState::MainMenu);
            } else if (modal_ == Modal::QuitGame) {
                modal_ = Modal::None;
                setState(UIState::MainMenu);
            }
            return;
        }
    }
    if (const auto* mm = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mm->button == sf::Mouse::Button::Left)
        {
            if (modal_ == Modal::ConfirmExit) {
                const sf::Vector2f mp = toUiCoords(mm->position);
                const sf::FloatRect yes({ UI_W / 2.f - 160, UI_H / 2.f + 20 }, { 140, 50 });
                const sf::FloatRect no({ UI_W / 2.f + 20, UI_H / 2.f + 20 }, { 140, 50 });
                if (yes.contains(mp)) { audio_.playUiClick(); win_.close(); }
                if (no.contains(mp)) { audio_.playUiClick(); modal_ = Modal::None; }
                return;
            }

            const sf::Vector2f mp(static_cast<float>(mm->position.x),
                                  static_cast<float>(mm->position.y));
            if (confirmationNoBounds().contains(mp)) { audio_.playUiClick(); modal_ = Modal::None; }
            if (confirmationYesBounds().contains(mp)) {
                audio_.playUiClick();
                if (modal_ == Modal::SaveGame) saveCurrentRun();
                modal_ = Modal::None;
                setState(UIState::MainMenu);
            }
        }
    }
}

/**
 * @brief Performs the sidebar pause bounds operation while preserving the current UI state invariants.
 */
sf::FloatRect UIManager::sidebarPauseBounds() const
{

    constexpr float textureWidth = 551.f;
    constexpr float textureHeight = 1012.f;
    const float sx = Grid::SIDEBAR_WIDTH / textureWidth;
    const float sy = Grid::MAP_HEIGHT / textureHeight;
    return { { Grid::MAP_WIDTH + 132.f * sx, 559.f * sy }, { 317.f * sx, 128.f * sy } };
}

/**
 * @brief Performs the sidebar save bounds operation while preserving the current UI state invariants.
 */
sf::FloatRect UIManager::sidebarSaveBounds() const
{
    constexpr float textureWidth = 551.f;
    constexpr float textureHeight = 1012.f;
    const float sx = Grid::SIDEBAR_WIDTH / textureWidth;
    const float sy = Grid::MAP_HEIGHT / textureHeight;
    return { { Grid::MAP_WIDTH + 132.f * sx, 720.f * sy }, { 317.f * sx, 128.f * sy } };
}

/**
 * @brief Performs the sidebar quit bounds operation while preserving the current UI state invariants.
 */
sf::FloatRect UIManager::sidebarQuitBounds() const
{
    constexpr float textureWidth = 551.f;
    constexpr float textureHeight = 1012.f;
    const float sx = Grid::SIDEBAR_WIDTH / textureWidth;
    const float sy = Grid::MAP_HEIGHT / textureHeight;
    return { { Grid::MAP_WIDTH + 132.f * sx, 884.f * sy }, { 317.f * sx, 128.f * sy } };
}

/**
 * @brief Performs the confirmation popup bounds operation while preserving the current UI state invariants.
 */
sf::FloatRect UIManager::confirmationPopupBounds() const
{

    const sf::Vector2u windowSize = win_.getSize();
    const float width = std::min(720.f, static_cast<float>(windowSize.x) * .72f);
    const float height = std::min(500.f, static_cast<float>(windowSize.y) * .72f);
    return { { (windowSize.x - width) * .5f, (windowSize.y - height) * .5f }, { width, height } };
}

/**
 * @brief Performs the confirmation yes bounds operation while preserving the current UI state invariants.
 */
sf::FloatRect UIManager::confirmationYesBounds() const
{
    const sf::FloatRect popup = confirmationPopupBounds();
    return { { popup.position.x + popup.size.x * .18f, popup.position.y + popup.size.y * .60f },
             { popup.size.x * .30f, popup.size.y * .19f } };
}


/**
 * @brief Performs the confirmation no bounds operation while preserving the current UI state invariants.
 */
sf::FloatRect UIManager::confirmationNoBounds() const
{
    const sf::FloatRect popup = confirmationPopupBounds();
    return { { popup.position.x + popup.size.x * .55f, popup.position.y + popup.size.y * .60f },
             { popup.size.x * .30f, popup.size.y * .19f } };
}

/**
 * @brief Advances state that depends on elapsed time after input has established the current intent.
 */
void UIManager::update(float dt)
{
    if (state_ == UIState::Loading) {
        if (!loadingTransitionActive_) {
            loadingBlinkElapsed_ = std::fmod(loadingBlinkElapsed_ + dt, LOADING_BLINK_PERIOD);
            return;
        }

        loadingZoomElapsed_ += dt;
        const float progress = std::min(loadingZoomElapsed_ / LOADING_ZOOM_DURATION, 1.f);
        const float easedProgress = 1.f - std::pow(1.f - progress, 3.f);
        loadingView_ = win_.getDefaultView();
        loadingView_.zoom(1.f + (LOADING_ZOOM_SCALE - 1.f) * easedProgress);
        if (progress >= 1.f) {
            win_.setView(win_.getDefaultView());
            setState(UIState::MainMenu);
        }
        return;
    }

    if (gameplayStarted_ && state_ == UIState::EndlessPlay && isEndlessPlayerOffscreen())
    {
        finishEndlessRun();
        return;
    }

    if (gameplayStarted_ && (state_ == UIState::ClassicPlay || state_ == UIState::EndlessPlay))
    {

        if (!audio_.vehicleAmbiencePlaying()) {
            audio_.startVehicleAmbience();
        }

        elapsedPlaySec_ += dt;
        updateCamera(dt);
        player_.setCameraOffset(cameraY_);
        player_.update(dt);

        const sf::Vector2f playerPos = player_.getPosition();

        if (state_ == UIState::ClassicPlay &&
            playerPos.y <= classicMap_.topLimit() + Grid::CELL_SIZE * 0.5f)
        {
            classicWon_ = true;
            ctx_.classicLevel = ctx_.level;
            ctx_.classicSec = static_cast<int>(elapsedPlaySec_);
            prog_.setHighestUnlockedLevel(std::max(prog_.highestUnlockedLevel(),
                std::min(CLASSIC_LEVELS, ctx_.level + 1)));
            setState(UIState::GameOver);
            return;
        }

        const int playerCol = static_cast<int>(std::floor((playerPos.x - Grid::GRID_LEFT) / Grid::CELL_SIZE));
        bool steppedOnManhole = false;

        auto checkManhole = [&](const auto& mapBlocks) {
            for (const auto& block : mapBlocks) {
                if (block.contains(playerPos.y)) {
                    int row = static_cast<int>((playerPos.y - block.startY) / Grid::CELL_SIZE);
                    if (row >= 0 && row < LANES_PER_BLOCK) {
                        if (Grid::isPlayableColumn(playerCol) && block.manholeCols[row] == playerCol) {
                            steppedOnManhole = true;
                        }
                    }
                    break;
                }
            }
            };

        if (state_ == UIState::ClassicPlay) checkManhole(classicMap_.getBlocks());
        else if (state_ == UIState::EndlessPlay) checkManhole(endlessMap_.getBlocks());

        if (steppedOnManhole) {
            audio_.playFallSound();
            audio_.pauseVehicleAmbience();
            player_.kill();

            // Endless and classic maps use different generation data, so the renderer follows the active mode rather than inferring it from assets.
    if (state_ == UIState::EndlessPlay) {
                finishEndlessRun();
            }
            else {
                classicWon_ = false;
                ctx_.classicLevel = ctx_.level;
                ctx_.classicSec = static_cast<int>(elapsedPlaySec_);
                setState(UIState::GameOver);
            }
            return;
        }

    }
    else if (state_ == UIState::Pause || state_ != UIState::ClassicPlay && state_ != UIState::EndlessPlay)
    {

        if (audio_.vehicleAmbiencePlaying()) {
            audio_.pauseVehicleAmbience();
        }
    }

    if (gameplayStarted_ && state_ == UIState::EndlessPlay)
    {
        endlessMap_.update(cameraY_);

        if (isEndlessPlayerOffscreen())
            finishEndlessRun();
    }

    if (state_ == UIState::ClassicPlay || state_ == UIState::EndlessPlay) {
        if (gameplayStarted_) {

            float currentMultiplier = 1.0f;
            if (state_ == UIState::EndlessPlay) {
                float distanceTraveled = std::abs(player_.getPosition().y + 1080.0f);
                if (distanceTraveled > 15000.f) currentMultiplier = 2.0f;
                else if (distanceTraveled > 5000.f) currentMultiplier = 1.5f;
            }

            for (auto obs : Obstacles) {
                obs->setSpeedMultiplier(currentMultiplier);
                obs->move(dt);
            }

            auto isOnAssignedHazardLane = [](const CObstacle* obstacle, const auto& blocks) {
                const sf::FloatRect bounds = obstacle->getBounds();
                const float centerY = bounds.position.y + bounds.size.y * 0.5f;
                for (const MapBlock& block : blocks) {
                    if (!block.contains(centerY)) continue;
                    const int row = static_cast<int>(std::floor(
                        (centerY - block.startY) / Grid::CELL_SIZE));
                    if (row < 0 || row >= LANES_PER_BLOCK) return false;

                    const LaneType lane = block.lanes[row];
                    return (dynamic_cast<const CVehicle*>(obstacle) != nullptr && lane == LaneType::Vehicle) ||
                        (dynamic_cast<const CAnimal*>(obstacle) != nullptr && lane == LaneType::Animal);
                }
                return false;
                };

            for (int i = 0; i < Obstacles.size(); ) {
                const bool validLane = state_ == UIState::EndlessPlay
                    ? isOnAssignedHazardLane(Obstacles[i], endlessMap_.getBlocks())
                    : isOnAssignedHazardLane(Obstacles[i], classicMap_.getBlocks());
                if (Obstacles[i]->isOffScreen() || !validLane) {
                    delete Obstacles[i];

                    Obstacles[i] = Obstacles.back();

                    Obstacles.pop_back();

                }
                else {
                    ++i;
                }
            }

            obstacleSpawnTimer_ -= dt;
            if (obstacleSpawnTimer_ <= 0.f) {
                obstacleSpawnTimer_ = 1.0f;

                auto spawnInBlocks = [&](const auto& blocks) {
                    for (const MapBlock& block : blocks) {
                        for (int row = 0; row < LANES_PER_BLOCK; ++row) {
                            const LaneType type = block.lanes[row];
                            if (type == LaneType::Safe) continue;

                            const float laneCenterY = Grid::rowCenter(block.startY, row);
                            if (laneCenterY < cameraY_ - Grid::CELL_SIZE ||
                                laneCenterY > cameraY_ + Grid::MAP_HEIGHT + Grid::CELL_SIZE)
                                continue;

                            direction dir = ((block.blockID + row) % 2 == 0) ? RIGHT : LEFT;

                            if (rand() % 100 < 50) {
                                float laneTopY = block.startY + (row * Grid::CELL_SIZE);
                                float rowY = laneTopY + (Grid::CELL_SIZE * 0.25f);
                                float spawnX = (dir == RIGHT) ? Grid::GRID_LEFT - 150.f : Grid::GRID_RIGHT + 150.f;

                                bool isBlocked = false;
                                for (auto obs : Obstacles) {
                                    const sf::FloatRect bounds = obs->getBounds();
                                    const float obstacleCenterY = bounds.position.y + bounds.size.y * 0.5f;
                                    if (std::abs(obstacleCenterY - laneCenterY) < 100.0f) {
                                        if (std::abs(obs->getX() - spawnX) < 500.f) {
                                            isBlocked = true;
                                            break;
                                        }
                                    }
                                }

                                if (!isBlocked) {
                                    if (type == LaneType::Vehicle) {

                                        if (row % 2 == 0) {
                                            Obstacles.push_back(new CCar(spawnX, laneCenterY - 30.f, dir));
                                        }
                                        else {
                                            Obstacles.push_back(new CTruck(spawnX, laneCenterY - 37.f, dir));
                                        }
                                    }
                                    else if (type == LaneType::Animal) {
                                        if (row % 2 == 0) {
                                            Obstacles.push_back(new CCat(spawnX, rowY, dir));

                                            if (rand() % 100 < 10) audio_.playAnimalSample(false);
                                        }
                                        else {
                                            Obstacles.push_back(new CDeer(spawnX, rowY - 10, dir));

                                            if (rand() % 100 < 3) audio_.playAnimalSample(true);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    };

                if (state_ == UIState::EndlessPlay) {
                    spawnInBlocks(endlessMap_.getBlocks());
                }
                else {
                    spawnInBlocks(classicMap_.getBlocks());
                }
            }

            trafficLightTimer_ += dt;
            if (currentLight_ == TrafficLight::Green && trafficLightTimer_ >= 4.0f) {
                currentLight_ = TrafficLight::Yellow;
                trafficLightTimer_ -= 4.0f;
                trafficLightSprite_.setTexture(texTrafficYellow_);
            }
            else if (currentLight_ == TrafficLight::Yellow && trafficLightTimer_ >= 1.0f) {
                currentLight_ = TrafficLight::Red;
                trafficLightTimer_ -= 1.0f;
                trafficLightSprite_.setTexture(texTrafficRed_);
            }
            else if (currentLight_ == TrafficLight::Red && trafficLightTimer_ >= 2.0f) {
                currentLight_ = TrafficLight::Green;
                trafficLightTimer_ -= 2.0f;
                trafficLightSprite_.setTexture(texTrafficGreen_);
            }

            for (auto obs : Obstacles) {
                if (CVehicle* vehicle = dynamic_cast<CVehicle*>(obs)) {
                    if (currentLight_ == TrafficLight::Red) {
                        vehicle->stop();
                    }
                    else {
                        vehicle->continueMoving();
                    }
                }
            }

            const sf::Vector2f playerPos = player_.getPosition();
            const int pCol = static_cast<int>(std::floor((playerPos.x - Grid::GRID_LEFT) / Grid::CELL_SIZE));

            auto checkShieldPickup = [&](auto& mapBlocks) {
                for (auto& block : mapBlocks) {
                    if (block.contains(playerPos.y)) {
                        int row = static_cast<int>((playerPos.y - block.startY) / Grid::CELL_SIZE);
                        if (row >= 0 && row < LANES_PER_BLOCK) {
                            if (Grid::isPlayableColumn(pCol) && block.shieldCols[row] == pCol) {
                                if (!player_.hasShield()) {
                                    player_.giveShield();
                                    audio_.playUiClick();
                                }
                                block.shieldCols[row] = -1;
                            }
                        }
                        break;
                    }
                }
                };

            if (state_ == UIState::EndlessPlay) checkShieldPickup(endlessMap_.getMutableBlocks());
            else checkShieldPickup(classicMap_.getMutableBlocks());

            CGameObject* hitObstacle = nullptr;

            for (auto obs : Obstacles) {
                if (CVehicle* v = dynamic_cast<CVehicle*>(obs)) {
                    if (player_.isImpact(v)) { hitObstacle = obs; break; }
                }
                else if (CAnimal* a = dynamic_cast<CAnimal*>(obs)) {
                    if (player_.isImpact(a)) { hitObstacle = obs; break; }
                }
            }

            if (hitObstacle) {
                if (player_.isInvincible()) {

                }
                else {

                    if (dynamic_cast<CCat*>(hitObstacle)) {
                        audio_.playAnimalSample(false);
                    }
                    else if (dynamic_cast<CDeer*>(hitObstacle)) {
                        audio_.playAnimalSample(true);
                    }
                    else {
                        audio_.playUiCrash();
                    }

                    if (player_.hasShield()) {
                        player_.consumeShield();
                    }
                    else {
                        player_.kill();
                        audio_.pauseVehicleAmbience();

                        if (state_ == UIState::ClassicPlay) {
                            ctx_.classicSec = static_cast<int>(elapsedPlaySec_);
                        }
                        else {
                            ctx_.endlessSec = static_cast<int>(elapsedPlaySec_);
                        }
                        setState(UIState::GameOver);
                    }
                }
            }
        }
    }
}

/**
 * @brief Changes the screen through one path to keep focus, controls, and transition context synchronized.
 */
void UIManager::setState(UIState s)
{

    const bool enteringClassic = s == UIState::ClassicPlay &&
                                 state_ != UIState::ClassicPlay && state_ != UIState::Pause;
    const bool enteringEndless = s == UIState::EndlessPlay &&
                                 state_ != UIState::EndlessPlay && state_ != UIState::Pause;

    const UIState previousState = state_;
    if (s == UIState::Pause &&
        (previousState == UIState::ClassicPlay || previousState == UIState::EndlessPlay))
        capturePausedFrame();

    state_ = s;
    if (s == UIState::Graphic && previousState != UIState::Graphic)
        beginGraphicPreview();
    if (enteringClassic || enteringEndless)
        resetGameplay();
    if (s == UIState::Pause || previousState == UIState::Pause)
        clock_.restart();
    focusIdx_ = 0;
    rebuildButtons();
}

/**
 * @brief Performs the reset gameplay operation while preserving the current UI state invariants.
 */
void UIManager::resetGameplay()
{
    cameraY_ = -Grid::MAP_HEIGHT;
    player_.setSpawnPosition({ Grid::columnCenter(Grid::COLUMNS / 2), -Grid::CELL_SIZE * 0.5f });
    player_.setSkin(ctx_.selectedCharacterID);
    player_.resetPosition();
    player_.setCameraOffset(cameraY_);
    elapsedPlaySec_ = 0.f;
    gameplayStarted_ = false;
    classicWon_ = false;

    if (state_ == UIState::EndlessPlay)
        endlessMap_.reset();
    else
        classicMap_.init(std::clamp(ctx_.level, 1, CLASSIC_LEVELS));

    for (auto obs : Obstacles) {
        delete obs;
    }
    Obstacles.clear();
    obstacleSpawnTimer_ = 0.f;
    currentLight_ = TrafficLight::Green;
    trafficLightTimer_ = 0.f;
    trafficLightSprite_.setTexture(texTrafficGreen_);
}

/**
 * @brief Performs the max walkable player y operation while preserving the current UI state invariants.
 */
float UIManager::maxWalkablePlayerY() const
{

    const float viewBottom = cameraY_ + Grid::MAP_HEIGHT;
    const float lastCompleteRowBottom = std::floor(viewBottom / Grid::CELL_SIZE) * Grid::CELL_SIZE;
    return lastCompleteRowBottom - Grid::CELL_SIZE * 0.5f;
}

/**
 * @brief Performs the is endless player offscreen operation while preserving the current UI state invariants.
 */
bool UIManager::isEndlessPlayerOffscreen() const
{
    return player_.getPosition().y - cameraY_ - player_.getBounds().size.y * 0.5f > Grid::MAP_HEIGHT;
}

/**
 * @brief Performs the finish endless run operation while preserving the current UI state invariants.
 */
void UIManager::finishEndlessRun()
{
    if (state_ != UIState::EndlessPlay) return;
    player_.kill();

    ctx_.endlessSec = static_cast<int>(elapsedPlaySec_);
    setState(UIState::GameOver);
}

/**
 * @brief Keeps the player readable on screen while allowing world coordinates to remain independent of the viewport.
 */
void UIManager::updateCamera(float dt)
{
    if (state_ == UIState::EndlessPlay)
    {

        cameraY_ -= ENDLESS_SCROLL_SPEED * dt;

        const float playerScreenY = player_.getPosition().y - cameraY_;
        if (playerScreenY < ENDLESS_CATCHUP_DISTANCE)
        {
            const float targetY = player_.getPosition().y - PLAYER_SCREEN_ANCHOR;
            if (targetY < cameraY_)
                cameraY_ = std::max(targetY, cameraY_ - ENDLESS_CATCHUP_SPEED * dt);

            const float safeCameraY = player_.getPosition().y - PLAYER_TOP_SAFE_LINE;
            if (cameraY_ > safeCameraY)
                cameraY_ = safeCameraY;
        }
        return;
    }

    const float stopY = classicMap_.topLimit();
    const float targetY = std::max(stopY, player_.getPosition().y - PLAYER_SCREEN_ANCHOR);
    if (targetY < cameraY_)
        cameraY_ = std::max(targetY, cameraY_ - CLASSIC_FOLLOW_SPEED * dt);

    const float safeCameraY = std::max(stopY, player_.getPosition().y - PLAYER_TOP_SAFE_LINE);
    if (cameraY_ > safeCameraY)
        cameraY_ = safeCameraY;
}

/**
 * @brief Converts pixels through the UI view so input follows letterboxing and view scaling.
 */
sf::Vector2f UIManager::toUiCoords(sf::Vector2i pixel) const
{
    return win_.mapPixelToCoords(pixel, uiView_);
}

/**
 * @brief Converts pixels into authored-art coordinates so one set of hitboxes works at every window size.
 */
sf::Vector2f UIManager::toBaseCoords(sf::Vector2i pixel) const
{
    const sf::Vector2u size = win_.getSize();
    if (size.x == 0 || size.y == 0) return {};
    return { pixel.x * (WINDOW_W / static_cast<float>(size.x)),
             pixel.y * (WINDOW_H / static_cast<float>(size.y)) };
}

/**
 * @brief Performs the scaled menu bounds operation while preserving the current UI state invariants.
 */
sf::FloatRect UIManager::scaledMenuBounds(std::size_t index) const
{
    return scaledBaseRect(kMainMenuButtonBounds.at(index));
}

/**
 * @brief Scales authored hitboxes with the artwork, preventing click targets from drifting after a resize.
 */
sf::FloatRect UIManager::scaledBaseRect(const sf::FloatRect& base) const
{
    const sf::Vector2u size = win_.getSize();
    const float sx = size.x / static_cast<float>(WINDOW_W);
    const float sy = size.y / static_cast<float>(WINDOW_H);
    return { { base.position.x * sx, base.position.y * sy },
             { base.size.x * sx, base.size.y * sy } };
}

/**
 * @brief Performs the set sfx volume from mouse operation while preserving the current UI state invariants.
 */
void UIManager::setSfxVolumeFromMouse(sf::Vector2i pixel)
{
    const sf::FloatRect track = scaledBaseRect(kSfxTrackBounds);
    const float ratio = (pixel.x - track.position.x) / track.size.x;
    cfg_.volume = std::clamp(static_cast<int>(std::lround(ratio * 100.f)), 0, 100);
    applyAudioVolumes();
}

/**
 * @brief Performs the set music volume from mouse operation while preserving the current UI state invariants.
 */
void UIManager::setMusicVolumeFromMouse(sf::Vector2i pixel)
{
    const sf::FloatRect track = scaledBaseRect(kMusicTrackBounds);
    const float ratio = (pixel.x - track.position.x) / track.size.x;
    cfg_.musicVolume = std::clamp(static_cast<int>(std::lround(ratio * 100.f)), 0, 100);
    applyAudioVolumes();
}

/**
 * @brief Applies both volume categories together so persisted settings and live audio remain consistent.
 */
void UIManager::applyAudioVolumes()
{

    audio_.setUserVolumes(static_cast<float>(cfg_.musicVolume),
                          static_cast<float>(cfg_.volume));
}

/**
 * @brief Performs the handle debug audio mixer key operation while preserving the current UI state invariants.
 */
bool UIManager::handleDebugAudioMixerKey(const sf::Event::KeyPressed& key)
{
    constexpr std::size_t categoryCount = static_cast<std::size_t>(AudioCategory::Count);
    switch (key.code)
    {
    case sf::Keyboard::Key::Up:
        selectedAudioCategory_ = (selectedAudioCategory_ + categoryCount - 1) % categoryCount;
        return true;
    case sf::Keyboard::Key::Down:
        selectedAudioCategory_ = (selectedAudioCategory_ + 1) % categoryCount;
        return true;
    case sf::Keyboard::Key::Left:
        audio_.adjustBaseVolume(static_cast<AudioCategory>(selectedAudioCategory_), -0.05f);
        return true;
    case sf::Keyboard::Key::Right:
        audio_.adjustBaseVolume(static_cast<AudioCategory>(selectedAudioCategory_), 0.05f);
        return true;
    case sf::Keyboard::Key::Space:
        audio_.playPreview(static_cast<AudioCategory>(selectedAudioCategory_));
        return true;
    default:
        return false;
    }
}

/**
 * @brief Performs the menu button at operation while preserving the current UI state invariants.
 */
int UIManager::menuButtonAt(sf::Vector2i pixel) const
{
    const sf::Vector2f point(static_cast<float>(pixel.x), static_cast<float>(pixel.y));
    for (std::size_t i = 0; i < kMainMenuButtonBounds.size(); ++i)
        if (scaledMenuBounds(i).contains(point)) return static_cast<int>(i);
    return -1;
}

/**
 * @brief Performs the activate main menu button operation while preserving the current UI state invariants.
 */
void UIManager::activateMainMenuButton(std::size_t index)
{
    focusIdx_ = static_cast<int>(index);
    switch (index)
    {
    case 0: setState(UIState::ModeSelect); break;
    case 1: loadTabModeIdx_ = 0; setState(UIState::LoadGame); break;
    case 2: setState(UIState::Graphic); break;
    case 3: setState(UIState::Setting); break;
    case 4: setState(UIState::Ranking); break;
    case 5: setState(UIState::Help); break;
    case 6: modal_ = Modal::ConfirmExit; break;
    default: break;
    }
}

/**
 * @brief Performs the move focus operation while preserving the current UI state invariants.
 */
void UIManager::moveFocus(int dir)
{
    if (btns_.empty()) return;
    int n = (int)btns_.size();
    for (int step = 0; step < n; ++step)
    {
        focusIdx_ = (focusIdx_ + dir + n) % n;
        if (btns_[focusIdx_].isEnabled()) break;
    }
    for (int i = 0; i < n; ++i) btns_[i].setFocused(i == focusIdx_);
}

/**
 * @brief Performs the activate focused operation while preserving the current UI state invariants.
 */
void UIManager::activateFocused()
{
    if (btns_.empty()) return;
    if (focusIdx_ < 0 || focusIdx_ >= (int)btns_.size()) return;
    if (!btns_[focusIdx_].isEnabled()) return;
    btns_[focusIdx_].consumeEnter();

    if (focusIdx_ < (int)btnActions_.size() && btnActions_[focusIdx_])
        btnActions_[focusIdx_]();
}

/**
 * @brief Builds controls after each transition so callbacks always belong to the active screen.
 */
void UIManager::rebuildButtons()
{
    btns_.clear();
    btnActions_.clear();

    auto add = [&](sf::Vector2f pos, sf::Vector2f size, const std::string& label,
                   Button::Style style, std::function<void()> action,
                   bool enabled = true)
    {
        Button b(font_, pos, size, label, style);
        b.setEnabled(enabled);
        btns_.push_back(b);
        btnActions_.push_back(std::move(action));
    };

    auto vstack = [&](float top, int count) {
        return [count, top](int i) -> sf::Vector2f {
            return { (UI_W - BTN_W) * 0.5f, top + i * (BTN_H + BTN_GAP) };
        };
    };

    switch (state_)
    {
    case UIState::MainMenu:
    {
        auto y = vstack(220, 7);
        add(y(0), { BTN_W, BTN_H }, "New Game", Button::Style::Primary, [this] { setState(UIState::ModeSelect); });
        add(y(1), { BTN_W, BTN_H }, "Load Game", Button::Style::Primary, [this] { loadTabModeIdx_ = 0; setState(UIState::LoadGame); });
        add(y(2), { BTN_W, BTN_H }, "Ranking", Button::Style::Primary, [this] { setState(UIState::Ranking); });
        add(y(3), { BTN_W, BTN_H }, "Setting", Button::Style::Primary, [this] { setState(UIState::Setting); });
        add(y(4), { BTN_W, BTN_H }, "Graphic", Button::Style::Primary, [this] { setState(UIState::Graphic); });
        add(y(5), { BTN_W, BTN_H }, "Help", Button::Style::Primary, [this] { setState(UIState::Help); });
        add(y(6), { BTN_W, BTN_H }, "Exit", Button::Style::Danger, [this] { modal_ = Modal::ConfirmExit; });
        break;
    }

    case UIState::ModeSelect:

        break;

    case UIState::LevelSelect:

        break;

    case UIState::Setting:
    {
        auto y = vstack(400, 1);
        add(y(0), { BTN_W, BTN_H }, "Back", Button::Style::Subtle, [this] { handleBack(); });
        break;
    }

    case UIState::Graphic:
        break;

    case UIState::LoadGame:
    {

        add({ 200, 150 }, { 200, 48 }, loadTabModeIdx_ == 0 ? "[X] Classic" : "[ ] Classic",
            Button::Style::Subtle, [this] { loadTabModeIdx_ = 0; rebuildButtons(); });
        add({ 420, 150 }, { 200, 48 }, loadTabModeIdx_ == 1 ? "[X] Endless" : "[ ] Endless",
            Button::Style::Subtle, [this] { loadTabModeIdx_ = 1; rebuildButtons(); });
        const GameMode m = (loadTabModeIdx_ == 0) ? GameMode::Classic : GameMode::Endless;
        const auto slots = saves_.slots(m);
        for (int i = 0; i < SaveStore::kMaxSlots; ++i)
        {
            const float x = 200.f + (i % 3) * 280.f;
            const float y = 260.f;
            std::string label = "Slot " + std::to_string(i + 1) + "  (empty)";

            if (!slots[i].name.empty())
            {
                label = "Slot " + std::to_string(i + 1) + ": " + slots[i].name
                    + "  L" + std::to_string(slots[i].level)
                    + "  " + std::to_string(slots[i].elapsedSec) + "s";
            }

            const bool filled = !slots[i].name.empty();

            add({ x, y }, { 240, 120 }, label, Button::Style::Primary, [this, m, i] {
                const auto s2 = saves_.slots(m);

                if (!s2[i].name.empty())
                {
                    ctx_.pendingName = s2[i].name;
                    ctx_.level = s2[i].level;
                    ctx_.classicLevel = s2[i].level;
                    ctx_.classicSec = s2[i].elapsedSec;
                    ctx_.mode = (m == GameMode::Endless)
                        ? StateContext::Mode::Endless
                        : StateContext::Mode::Classic;

                    setState(m == GameMode::Endless
                        ? UIState::EndlessPlay
                        : UIState::ClassicPlay);

                    player_.setPosition({ s2[i].playerX, s2[i].playerY });
                    player_.revive();
                    cameraY_ = s2[i].cameraY;
                    player_.setCameraOffset(cameraY_);
                    elapsedPlaySec_ = static_cast<float>(s2[i].elapsedSec);
                }
                }, filled);
        }
        break;
    }

    case UIState::Ranking: break;

    case UIState::Help:
        break;

    default:
        break;
    }

    for (int i = 0; i < (int)btns_.size(); ++i) btns_[i].setFocused(i == focusIdx_);
}

/**
 * @brief Performs the is valid name char operation while preserving the current UI state invariants.
 */
bool UIManager::isValidNameChar(std::uint32_t c)
{
    if (c == ' ') return true;
    if (c >= '0' && c <= '9') return true;
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= 'a' && c <= 'z') return true;
    if (c == '_' || c == '-' || c == '.') return true;
    return false;
}

/**
 * @brief Performs the is valid name operation while preserving the current UI state invariants.
 */
bool UIManager::isValidName(const std::string& s)
{
    if (s.empty() || (int)s.size() > NAME_MAX) return false;
    for (char c : s) if (!isValidNameChar((std::uint32_t)c)) return false;
    return true;
}

/**
 * @brief Performs the draw background operation while preserving the current UI state invariants.
 */
void UIManager::drawBackground()
{
    if (assetsLoaded_)
    {
        if (state_ == UIState::GameOver) {
            const sf::Texture& resultTexture = classicWon_ ? winTex_ : loseTex_;
            const bool resultLoaded = classicWon_ ? winAssetLoaded_ : loseAssetLoaded_;
            if (resultLoaded && resultTexture.getSize().x != 0 && resultTexture.getSize().y != 0) {
                sf::Sprite result(resultTexture);
                result.setPosition(resultScreenBounds().position);
                win_.draw(result);
                return;
            }
        }
        const sf::Texture* texture = &bgTex_;
        if (state_ == UIState::Setting) texture = &settingBgTex_;
        else if (state_ == UIState::Graphic) texture = &graphicBgTex_;
        if (state_ == UIState::LevelSelect && levelBgAssetLoaded_) {
            const sf::FloatRect bounds = levelScreenBounds();
            sf::Sprite image(levelBgTex_);
            image.setPosition(bounds.position);
            win_.draw(image);
            return;
        }
        if (state_ == UIState::NameInput && userNameBgAssetLoaded_) {
            const sf::FloatRect bounds = userNameScreenBounds();
            sf::Sprite image(userNameBgTex_);
            image.setPosition(bounds.position);
            win_.draw(image);
            return;
        }
        if (state_ == UIState::ModeSelect && gameModeBgAssetLoaded_) {
            const sf::FloatRect bounds = gameModeScreenBounds();
            sf::Sprite image(gameModeBgTex_);
            image.setPosition(bounds.position);
            win_.draw(image);
            return;
        }
        else if (state_ == UIState::LoadGame && loadBgTex_.getSize().x != 0) texture = &loadBgTex_;
        else if (state_ == UIState::Ranking) texture = &rankingBgTex_;

        const sf::Vector2u windowSize = win_.getSize();
        const sf::Vector2u textureSize = texture->getSize();
        if (textureSize.x == 0 || textureSize.y == 0) return;
        sf::Sprite s(*texture);
        s.setScale({ windowSize.x / static_cast<float>(textureSize.x),
                     windowSize.y / static_cast<float>(textureSize.y) });
        win_.draw(s);
    }
    else
    {
        win_.clear(colorFromHex(0x12121A));
    }
}

/**
 * @brief Performs the draw main menu debug overlay operation while preserving the current UI state invariants.
 */
void UIManager::drawMainMenuDebugOverlay()
{
    static const std::array<std::string, 7> labels = {
        "Play", "Load", "Graphic", "Setting", "Leaderboard", "Help", "Exit"
    };
    const sf::Vector2i mouse = sf::Mouse::getPosition(win_);
    const sf::Vector2f point(static_cast<float>(mouse.x), static_cast<float>(mouse.y));

    for (std::size_t i = 0; i < kMainMenuButtonBounds.size(); ++i)
    {
        const sf::FloatRect bounds = scaledMenuBounds(i);
        sf::RectangleShape outline(bounds.size);
        outline.setPosition(bounds.position);
        outline.setFillColor(sf::Color::Transparent);
        outline.setOutlineThickness(2.f);
        outline.setOutlineColor(bounds.contains(point) ? sf::Color::Red : sf::Color::Green);
        win_.draw(outline);

        sf::Text label(font_, labels[i], 16);
        label.setFillColor(sf::Color::White);
        label.setPosition({ bounds.position.x + 4.f, bounds.position.y - 22.f });
        win_.draw(label);
    }
}

/**
 * @brief Performs the draw main menu hover glow operation while preserving the current UI state invariants.
 */
void UIManager::drawMainMenuHoverGlow()
{
    const int hovered = menuButtonAt(sf::Mouse::getPosition(win_));
    if (hovered < 0) return;

    const sf::FloatRect bounds = scaledMenuBounds(static_cast<std::size_t>(hovered));
    if (hovered < 4)
    {
        sf::RectangleShape glow(bounds.size);
        glow.setPosition(bounds.position);
        glow.setFillColor(sf::Color(255, 215, 0, 90));
        win_.draw(glow);
    }
    else
    {
        const float radius = std::min(bounds.size.x, bounds.size.y) * 0.5f;
        sf::CircleShape glow(radius);
        glow.setPosition({ bounds.position.x + (bounds.size.x - radius * 2.f) * 0.5f,
                           bounds.position.y + (bounds.size.y - radius * 2.f) * 0.5f });
        glow.setFillColor(sf::Color(255, 255, 255, 90));
        win_.draw(glow);
    }
}

/**
 * @brief Performs the draw mouse debug info operation while preserving the current UI state invariants.
 */
void UIManager::drawMouseDebugInfo()
{
    if (!fontLoaded_) return;

    const sf::Vector2i mouse = sf::Mouse::getPosition(win_);
    const sf::Vector2f base = toBaseCoords(mouse);
    const std::string info = "Screen: (" + std::to_string(mouse.x) + ", " + std::to_string(mouse.y) + ")\n"
                           + "Base: (" + std::to_string(static_cast<int>(std::lround(base.x))) + ", "
                           + std::to_string(static_cast<int>(std::lround(base.y))) + ")";
    debugText_.setString(info);
    debugText_.setPosition({ static_cast<float>(mouse.x) + 15.f,
                             static_cast<float>(mouse.y) + 15.f });
    win_.draw(debugText_);
}

/**
 * @brief Performs the draw debug audio mixer operation while preserving the current UI state invariants.
 */
void UIManager::drawDebugAudioMixer()
{
    if (!fontLoaded_) return;

    const sf::Vector2u windowSize = win_.getSize();
    constexpr float panelWidth = 370.f;
    constexpr float rowHeight = 38.f;
    constexpr float panelPadding = 20.f;
    const float panelHeight = 310.f;
    const float panelX = std::max(12.f, static_cast<float>(windowSize.x) - panelWidth - 18.f);
    const float panelY = 18.f;

    sf::RectangleShape panel({ panelWidth, panelHeight });
    panel.setPosition({ panelX, panelY });
    panel.setFillColor(sf::Color(12, 17, 25, 230));
    panel.setOutlineThickness(2.f);
    panel.setOutlineColor(sf::Color(255, 190, 60));
    win_.draw(panel);

    sf::Text title(font_, "DEBUG AUDIO MIXER", 23);
    title.setFillColor(sf::Color(255, 210, 80));
    title.setPosition({ panelX + panelPadding, panelY + 14.f });
    win_.draw(title);

    sf::Text subtitle(font_, "Base Volume (developer only)", 15);
    subtitle.setFillColor(sf::Color(185, 195, 210));
    subtitle.setPosition({ panelX + panelPadding, panelY + 48.f });
    win_.draw(subtitle);

    constexpr std::size_t categoryCount = static_cast<std::size_t>(AudioCategory::Count);
    for (std::size_t i = 0; i < categoryCount; ++i)
    {
        const float y = panelY + 78.f + static_cast<float>(i) * rowHeight;
        const AudioCategory category = static_cast<AudioCategory>(i);
        const bool selected = i == selectedAudioCategory_;
        const float value = audio_.baseVolume(category);

        sf::RectangleShape row({ panelWidth - panelPadding * 2.f, rowHeight - 4.f });
        row.setPosition({ panelX + panelPadding, y });
        row.setFillColor(selected ? sf::Color(54, 79, 108, 235) : sf::Color(28, 35, 47, 190));
        win_.draw(row);

        sf::Text label(font_, std::string(selected ? "> " : "  ") +
                                  AudioManager::displayName(category), 18);
        label.setFillColor(selected ? sf::Color::White : sf::Color(205, 210, 220));
        label.setPosition({ panelX + panelPadding + 8.f, y + 4.f });
        win_.draw(label);

        const std::string percent = std::to_string(static_cast<int>(std::lround(value * 100.f))) + "%";
        sf::Text valueText(font_, percent, 18);
        valueText.setFillColor(selected ? sf::Color(255, 220, 90) : sf::Color(180, 220, 255));
        const sf::FloatRect valueBounds = valueText.getLocalBounds();
        valueText.setPosition({ panelX + panelWidth - panelPadding - valueBounds.size.x - valueBounds.position.x - 8.f,
                                y + 4.f });
        win_.draw(valueText);
    }

    sf::Text help(font_, "Up/Down: select   Left/Right: -/+5%\nSpace: preview   F8: close", 15);
    help.setFillColor(sf::Color(185, 195, 210));
    help.setPosition({ panelX + panelPadding, panelY + panelHeight - 53.f });
    win_.draw(help);
}

/**
 * @brief Performs the draw active debug hitboxes operation while preserving the current UI state invariants.
 */
void UIManager::drawActiveDebugHitboxes()
{
    const sf::Vector2i mouse = sf::Mouse::getPosition(win_);
    const sf::Vector2f point(mouse.x, mouse.y);
    auto box = [&](const sf::FloatRect& base) {
        const sf::FloatRect r = scaledBaseRect(base);
        sf::RectangleShape shape(r.size); shape.setPosition(r.position);
        shape.setFillColor(sf::Color::Transparent); shape.setOutlineThickness(2.f);
        shape.setOutlineColor(r.contains(point) ? sf::Color::Green : sf::Color::Red);
        win_.draw(shape);
    };
    if (state_ == UIState::ModeSelect) {
        for (std::size_t i = 0; i < kGameModeButtonBounds.size(); ++i) {
            const sf::FloatRect bounds = gameModeButtonBounds(i);
            sf::RectangleShape shape(bounds.size);
            shape.setPosition(bounds.position);
            shape.setFillColor(sf::Color::Transparent);
            shape.setOutlineThickness(2.f);
            shape.setOutlineColor(bounds.contains(point) ? sf::Color::Green : sf::Color::Red);
            win_.draw(shape);
        }
    } else if (state_ == UIState::GameOver) {
        const sf::FloatRect bounds = resultHomeBounds();
        sf::RectangleShape shape(bounds.size);
        shape.setPosition(bounds.position);
        shape.setFillColor(sf::Color::Transparent);
        shape.setOutlineThickness(2.f);
        shape.setOutlineColor(bounds.contains(point) ? sf::Color::Green : sf::Color::Red);
        win_.draw(shape);
    } else if (state_ == UIState::Graphic) {
        box(kCharacterPanelBounds); box(kCharacterPrevBounds); box(kCharacterNextBounds);
        box(kThemePanelBounds); box(kThemePrevBounds); box(kThemeNextBounds);
        box(kGraphicCharacterOkBounds);
        box(kGraphicOkThemeBounds);
    } else if (state_ == UIState::Setting) {
        box(kSfxTrackBounds); box(kSfxDecBounds); box(kSfxIncBounds);
        box(kMusicTrackBounds); box(kMusicDecBounds); box(kMusicIncBounds); box(kSettingOkBounds);
    } else if (state_ == UIState::LevelSelect) {
        for (std::size_t i = 0; i < kLevelButtonBounds.size(); ++i) {
            const sf::FloatRect bounds = levelButtonBounds(i);
            sf::RectangleShape shape(bounds.size);
            shape.setPosition(bounds.position);
            shape.setFillColor(sf::Color::Transparent);
            shape.setOutlineThickness(2.f);
            shape.setOutlineColor(bounds.contains(point) ? sf::Color::Green : sf::Color::Red);
            win_.draw(shape);
        }
    } else if (state_ == UIState::NameInput) {
        for (const sf::FloatRect& bounds : { userNameInputBounds(), userNameConfirmBounds() }) {
            sf::RectangleShape shape(bounds.size);
            shape.setPosition(bounds.position);
            shape.setFillColor(sf::Color::Transparent);
            shape.setOutlineThickness(2.f);
            shape.setOutlineColor(bounds.contains(point) ? sf::Color::Green : sf::Color::Red);
            win_.draw(shape);
        }
    }

    if (state_ != UIState::Loading && state_ != UIState::MainMenu && state_ != UIState::Pause && state_ != UIState::GameOver &&
        state_ != UIState::ClassicPlay && state_ != UIState::EndlessPlay)
        box(sf::FloatRect({ 1721.f, 24.f }, { 175.f, 77.f }));

    if (state_ == UIState::Pause) {
        for (const sf::FloatRect& physical : { pauseResumeBounds(), pauseOutBounds() }) {
            sf::RectangleShape shape(physical.size);
            shape.setPosition(physical.position);
            shape.setFillColor(sf::Color::Transparent);
            shape.setOutlineThickness(2.f);
            shape.setOutlineColor(physical.contains(point) ? sf::Color::Green : sf::Color::Red);
            win_.draw(shape);
        }
    }

    if (state_ == UIState::ClassicPlay || state_ == UIState::EndlessPlay)
        drawGameplaySidebarHitboxes();

    if (modal_ == Modal::SaveGame || modal_ == Modal::QuitGame) {
        const sf::Vector2i mouse = sf::Mouse::getPosition(win_);
        const sf::Vector2f point(mouse.x, mouse.y);
        for (const sf::FloatRect& bounds : { confirmationYesBounds(), confirmationNoBounds() }) {
            sf::RectangleShape shape(bounds.size);
            shape.setPosition(bounds.position);
            shape.setFillColor(sf::Color::Transparent);
            shape.setOutlineThickness(2.f);
            shape.setOutlineColor(bounds.contains(point) ? sf::Color::Green : sf::Color::Red);
            win_.draw(shape);
        }
    }
}

/**
 * @brief Performs the draw gameplay sidebar hitboxes operation while preserving the current UI state invariants.
 */
void UIManager::drawGameplaySidebarHitboxes()
{
    const sf::Vector2i mouse = sf::Mouse::getPosition(win_);
    const sf::Vector2f point(mouse.x, mouse.y);
    for (const sf::FloatRect& bounds : { sidebarPauseBounds(), sidebarSaveBounds(), sidebarQuitBounds() }) {
        sf::RectangleShape shape(bounds.size);
        shape.setPosition(bounds.position);
        shape.setFillColor(sf::Color::Transparent);
        shape.setOutlineThickness(2.f);
        shape.setOutlineColor(bounds.contains(point) ? sf::Color::Green : sf::Color::Red);
        win_.draw(shape);
    }
}

/**
 * @brief Performs the draw back icon operation while preserving the current UI state invariants.
 */
void UIManager::drawBackIcon()
{
    if (state_ == UIState::Loading || state_ == UIState::MainMenu || state_ == UIState::Pause || state_ == UIState::GameOver ||
        state_ == UIState::ClassicPlay || state_ == UIState::EndlessPlay)
        return;

    const sf::FloatRect base({ 1721.f, 24.f }, { 175.f, 77.f });
    // Scale authored coordinates at draw time so a single layout remains valid for every window size.
        const sf::FloatRect bounds = scaledBaseRect(base);
    win_.setView(win_.getDefaultView());
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
    {
        const sf::Vector2i mouse = sf::Mouse::getPosition(win_);
        if (bounds.contains({ static_cast<float>(mouse.x), static_cast<float>(mouse.y) })) handleBack();
    }
    const sf::Vector2u textureSize = backButtonTex_.getSize();
    if (textureSize.x != 0 && textureSize.y != 0) {
        sf::Sprite back(backButtonTex_);
        back.setScale({ bounds.size.x / textureSize.x, bounds.size.y / textureSize.y });
        back.setPosition(bounds.position);
        win_.draw(back);
    }
    const sf::Vector2i mouse = sf::Mouse::getPosition(win_);
    e_isMouseOverBack = bounds.contains({ static_cast<float>(mouse.x), static_cast<float>(mouse.y) });
    win_.setView(uiView_);
}

/**
 * @brief Performs the draw modal overlay operation while preserving the current UI state invariants.
 */
void UIManager::drawModalOverlay()
{
    if (modal_ == Modal::None) return;

    if (modal_ == Modal::SaveGame || modal_ == Modal::QuitGame)
    {
        win_.setView(win_.getDefaultView());
        const sf::Vector2u windowSize = win_.getSize();
        sf::RectangleShape dim({ static_cast<float>(windowSize.x), static_cast<float>(windowSize.y) });
        dim.setFillColor(sf::Color(0, 0, 0, 160));
        win_.draw(dim);

        const sf::Texture& texture = modal_ == Modal::SaveGame ? saveTex_ : quitTex_;
        const bool textureLoaded = modal_ == Modal::SaveGame ? saveAssetLoaded_ : quitAssetLoaded_;
        const sf::FloatRect popup = confirmationPopupBounds();
        if (textureLoaded && texture.getSize().x != 0 && texture.getSize().y != 0) {
            sf::Sprite prompt(texture);
            prompt.setScale({ popup.size.x / texture.getSize().x,
                              popup.size.y / texture.getSize().y });
            prompt.setPosition(popup.position);
            win_.draw(prompt);
        }
        win_.setView(uiView_);
        return;
    }

    sf::RectangleShape dim({ UI_W, UI_H });
    dim.setFillColor(sf::Color(0, 0, 0, 160));
    win_.draw(dim);

    sf::FloatRect box({ UI_W/2.f - 250, UI_H/2.f - 80 }, { 500, 200 });
    sf::RectangleShape bg(box.size);
    bg.setPosition(box.position);
    bg.setFillColor(sf::Color(40, 40, 50, 240));
    bg.setOutlineThickness(-2.f);
    bg.setOutlineColor(sf::Color(200, 200, 220));
    win_.draw(bg);

    drawCenteredText("Are you sure you want to quit?", UI_H/2.f - 30.f, 24, sf::Color::White, true);

    Button yes(font_, { UI_W/2.f - 160, UI_H/2.f + 20 }, { 140, 50 }, "Yes",
               Button::Style::Danger);
    Button no (font_, { UI_W/2.f +  20, UI_H/2.f + 20 }, { 140, 50 }, "No",
               Button::Style::Subtle);
    yes.setFocused(modalFocus_ == 0);
    no .setFocused(modalFocus_ == 1);
    yes.draw(win_, font_);
    no .draw(win_, font_);
}

void UIManager::drawCenteredText(const std::string& s, float y, unsigned int size,
                                 sf::Color color, bool bold)
{
    sf::Text t(font_, s, size);
    t.setFillColor(color);
    if (bold) t.setStyle(sf::Text::Bold);
    const sf::FloatRect b = t.getLocalBounds();
    t.setPosition({ (UI_W - b.size.x) * 0.5f - b.position.x, y - b.position.y });
    win_.draw(t);
}

/**
 * @brief Draws the active screen before any modal, preserving the modal as the topmost interaction layer.
 */
void UIManager::render()
{
    const bool loading = state_ == UIState::Loading;
    const bool gameplay = state_ == UIState::ClassicPlay || state_ == UIState::EndlessPlay;
    const bool paused = state_ == UIState::Pause;
    if (paused)
    {

        win_.setView(win_.getDefaultView());
        win_.clear(sf::Color(21, 25, 31));
        if (pauseFrameValid_ && pauseFrameTex_.getSize().x != 0) {
            sf::Sprite frozenFrame(pauseFrameTex_);
            const sf::Vector2u frameSize = pauseFrameTex_.getSize();
            const sf::Vector2u windowSize = win_.getSize();
            frozenFrame.setScale({ windowSize.x / static_cast<float>(frameSize.x),
                                   windowSize.y / static_cast<float>(frameSize.y) });
            win_.draw(frozenFrame);
        }
        win_.setView(uiView_);
    }
    else if (loading)
    {
        win_.setView(loadingView_);
        win_.clear(sf::Color::Black);
    }
    else if (gameplay)
    {
        win_.setView(win_.getDefaultView());
        win_.clear(sf::Color(21, 25, 31));
    }
    else
    {

        win_.setView(win_.getDefaultView());
        drawBackground();
        win_.setView(uiView_);
    }

    switch (state_)
    {
    case UIState::Loading:     renderLoading();     break;
    case UIState::MainMenu:    renderMainMenu();    break;
    case UIState::ModeSelect:  renderModeSel();     break;
    case UIState::NameInput:   renderName();        break;
    case UIState::LevelSelect: renderLvlSel();      break;
    case UIState::Setting:     renderSetting();     break;
    case UIState::Graphic:     renderGraphic();     break;
    case UIState::LoadGame:    renderLoad();        break;
    case UIState::Ranking:     renderRanking();     break;
    case UIState::Help:        renderHelp();        break;
    case UIState::ClassicPlay:
    case UIState::EndlessPlay: renderPlay();        break;
    case UIState::GameOver:    renderGameOver();    break;
    case UIState::Pause:       renderPause();       break;
    }

    if (!gameplay && !loading) {
        drawBackIcon();
    }
    if (modal_ != Modal::None)
        drawModalOverlay();

    if (state_ == UIState::MainMenu)
    {
        win_.setView(win_.getDefaultView());
        drawMainMenuHoverGlow();
    }

    if (debugUi_)
    {
        win_.setView(win_.getDefaultView());
        if (state_ == UIState::MainMenu)
            drawMainMenuDebugOverlay();
        else
            drawActiveDebugHitboxes();
        drawMouseDebugInfo();
    }
    if (debugAudioMixer_)
    {
        win_.setView(win_.getDefaultView());
        drawDebugAudioMixer();
    }
    win_.display();
}

/**
 * @brief Draws the startup artwork and the blinking start prompt.
 */
void UIManager::renderLoading()
{
    if (loadingTex_.getSize().x != 0 && loadingTex_.getSize().y != 0) {
        sf::Sprite background(loadingTex_);
        const sf::Vector2u size = loadingTex_.getSize();
        background.setScale({ WINDOW_W / static_cast<float>(size.x),
                              WINDOW_H / static_cast<float>(size.y) });
        win_.draw(background);
    }

    if (loadingTransitionActive_)
        return;

    const float phase = loadingBlinkElapsed_ / LOADING_BLINK_PERIOD;
    const float alpha = 48.f + 207.f * (0.5f + 0.5f * std::sin(phase * 2.f * 3.14159265f));
    const auto opacity = static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 255.f));

    sf::RectangleShape promptBand({ static_cast<float>(WINDOW_W), LOADING_PROMPT_HEIGHT });
    promptBand.setPosition({ 0.f, static_cast<float>(WINDOW_H) - LOADING_PROMPT_HEIGHT });
    promptBand.setFillColor(sf::Color(0, 0, 0, opacity));
    win_.draw(promptBand);

    sf::Text prompt(font_, "Press \"Enter\" or click anywhere to start game", 34);
    prompt.setFillColor(sf::Color(255, 255, 255, opacity));
    const sf::FloatRect bounds = prompt.getLocalBounds();
    prompt.setPosition({ (WINDOW_W - bounds.size.x) * 0.5f - bounds.position.x,
                         WINDOW_H - LOADING_PROMPT_HEIGHT * 0.5f - (bounds.position.y + bounds.size.y * 0.5f) });
    win_.draw(prompt);
}

/**
 * @brief Performs the render main menu operation while preserving the current UI state invariants.
 */
void UIManager::renderMainMenu()
{

}

/**
 * @brief Performs the render mode sel operation while preserving the current UI state invariants.
 */
void UIManager::renderModeSel()
{

}

/**
 * @brief Performs the render name operation while preserving the current UI state invariants.
 */
void UIManager::renderName()
{
    win_.setView(win_.getDefaultView());
    const sf::FloatRect input = userNameInputBounds();
    const sf::FloatRect confirm = userNameConfirmBounds();
    sf::RectangleShape box(input.size);
    box.setPosition(input.position);
    box.setFillColor(sf::Color(28, 82, 156, 185));
    box.setOutlineThickness(3.f);
    box.setOutlineColor(sf::Color(225, 248, 255));
    win_.draw(box);

    sf::Text prompt(font_, "Enter player name", 25);
    prompt.setFillColor(sf::Color(22, 78, 143));
    prompt.setPosition({ input.position.x, input.position.y - 42.f });
    win_.draw(prompt);

    sf::Text t(font_, nameBuffer_ + "_", 32);
    t.setFillColor(sf::Color::White);
    t.setPosition({ input.position.x + 20.f, input.position.y + 21.f });
    win_.draw(t);

    sf::Text confirmText(font_, "CONFIRM", 29);
    confirmText.setFillColor(sf::Color::White);
    confirmText.setOutlineColor(sf::Color(30, 95, 170));
    confirmText.setOutlineThickness(2.f);
    const sf::FloatRect confirmTextBounds = confirmText.getLocalBounds();
    confirmText.setPosition({ confirm.position.x + (confirm.size.x - confirmTextBounds.size.x) * .5f - confirmTextBounds.position.x,
                              confirm.position.y + (confirm.size.y - confirmTextBounds.size.y) * .5f - confirmTextBounds.position.y });
    win_.draw(confirmText);
    win_.setView(uiView_);
}

/**
 * @brief Performs the render lvl sel operation while preserving the current UI state invariants.
 */
void UIManager::renderLvlSel()
{

    win_.setView(win_.getDefaultView());
    for (std::size_t i = 0; i < kLevelButtonBounds.size(); ++i)
    {
        const sf::FloatRect bounds = levelButtonBounds(i);
        sf::RectangleShape card(bounds.size);
        card.setPosition(bounds.position);
        card.setFillColor(sf::Color(12, 55, 125, 125));
        card.setOutlineThickness(4.f);
        card.setOutlineColor(sf::Color(205, 240, 255));
        win_.draw(card);

        // IconTheme is loaded once by setTheme(). Preserve its aspect ratio
        // and leave a small inset so it does not cover the card border.
        if (iconThemeTex_.getSize().x != 0 && iconThemeTex_.getSize().y != 0) {
            sf::Sprite icon(iconThemeTex_);
            const sf::Vector2u iconSize = iconThemeTex_.getSize();
            const float inset = 16.f;
            const float fit = std::min((bounds.size.x - inset * 2.f) / iconSize.x,
                                       (bounds.size.y - inset * 2.f) / iconSize.y);
            icon.setScale({ fit, fit });
            icon.setOrigin({ iconSize.x * .5f, iconSize.y * .5f });
            icon.setPosition({ bounds.position.x + bounds.size.x * .5f,
                               bounds.position.y + bounds.size.y * .5f });
            win_.draw(icon);
        }

        sf::Text number(font_, std::to_string(i + 1),
                        32);
        number.setFillColor(sf::Color::White);
        number.setOutlineColor(sf::Color(20, 80, 155));
        number.setOutlineThickness(2.f);
        const sf::FloatRect textBounds = number.getLocalBounds();
        number.setPosition({ bounds.position.x + (bounds.size.x - textBounds.size.x) * .5f - textBounds.position.x,
                             bounds.position.y + bounds.size.y + 12.f - textBounds.position.y });
        win_.draw(number);
    }
    win_.setView(uiView_);
}

/**
 * @brief Performs the render setting operation while preserving the current UI state invariants.
 */
void UIManager::renderSetting()
{
    win_.setView(win_.getDefaultView());
    auto slider = [&](const sf::FloatRect& base, int value, const std::string& name) {
        const sf::FloatRect b = scaledBaseRect(base);
        sf::RectangleShape track(b.size); track.setPosition(b.position); track.setFillColor(sf::Color(20,20,20,190)); win_.draw(track);
        sf::RectangleShape fill({b.size.x * value / 100.f, b.size.y}); fill.setPosition(b.position); fill.setFillColor(sf::Color(255,215,0,180)); win_.draw(fill);
        if (settingButtonTex_.getSize().x != 0 && settingButtonTex_.getSize().y != 0) {
            sf::Sprite knob(settingButtonTex_);
            const sf::Vector2u knobSize = settingButtonTex_.getSize();
            const float knobWidth = std::max(21.f, b.size.y * 1.4f);
            const float knobHeight = knobWidth * 10.f / 7.f;
            knob.setOrigin({ knobSize.x * 0.5f, knobSize.y * 0.5f });
            knob.setScale({ knobWidth / knobSize.x, knobHeight / knobSize.y });
            knob.setPosition({ b.position.x + b.size.x * value / 100.f, b.position.y + b.size.y * .5f });
            win_.draw(knob);
        }
        sf::Text label(font_, name + ": " + std::to_string(value) + "%", 22); label.setFillColor(sf::Color::White); label.setPosition({b.position.x,b.position.y-32.f}); win_.draw(label);
    };
    slider(kSfxTrackBounds, cfg_.volume, "SFX");
    slider(kMusicTrackBounds, cfg_.musicVolume, "Music");
    win_.setView(uiView_);
}

/**
 * @brief Performs the render graphic operation while preserving the current UI state invariants.
 */
void UIManager::renderGraphic()
{
    win_.setView(win_.getDefaultView());
    const sf::FloatRect characterPanel = scaledBaseRect(kCharacterPanelBounds);
    const sf::FloatRect themePanel = scaledBaseRect(kThemePanelBounds);

    CharacterRenderer::draw(win_, previewCharacterId_,
                            { characterPanel.position.x + characterPanel.size.x * .5f,
                              characterPanel.position.y + characterPanel.size.y * .5f },
                            std::min(characterPanel.size.x, characterPanel.size.y) * .22f);

    if (iconThemeTex_.getSize().x != 0 && iconThemeTex_.getSize().y != 0) {
        sf::Sprite icon(iconThemeTex_);
        const sf::Vector2u iconSize = iconThemeTex_.getSize();
        const float fit = std::min(themePanel.size.x / static_cast<float>(iconSize.x),
                                   themePanel.size.y / static_cast<float>(iconSize.y));
        icon.setOrigin({ iconSize.x * .5f, iconSize.y * .5f });
        icon.setScale({ fit, fit });
        icon.setPosition({ themePanel.position.x + themePanel.size.x * .5f,
                           themePanel.position.y + themePanel.size.y * .5f });
        win_.draw(icon);
    }
    win_.setView(uiView_);
}

/**
 * @brief Performs the render load operation while preserving the current UI state invariants.
 */
void UIManager::renderLoad()
{
    constexpr std::array<float, 3> rowY = { 286.f, 410.f, 536.f };
    constexpr std::array<sf::Color, 6> debugColors = {
        sf::Color::Red, sf::Color::Green, sf::Color::Blue,
        sf::Color::Yellow, sf::Color::Magenta, sf::Color::Cyan
    };

    auto drawColumn = [&](GameMode mode, float x, int colorOffset) {
        const auto slots = saves_.slots(mode);
        for (int place = 0; place < SaveStore::kMaxSlots; ++place) {
            const bool hasSave = !slots[place].name.empty();
            const std::string name = hasSave ? slots[place].name : "---";
            const std::string time = hasSave ? formatSaveTime(slots[place]) : "--";
            sf::Text entry(rankingFont_, name + "\nTime: " + time, 20);
            entry.setFillColor(sf::Color::White);
            entry.setPosition({ x, rowY[place] });
            win_.draw(entry);

            if (debugUi_) {
                const sf::FloatRect& baseBounds = kLoadSlotClickBounds[
                    colorOffset + place];
                const sf::FloatRect bounds(
                    { baseBounds.position.x * UI_W / WINDOW_W,
                      baseBounds.position.y * UI_H / WINDOW_H },
                    { baseBounds.size.x * UI_W / WINDOW_W,
                      baseBounds.size.y * UI_H / WINDOW_H });
                sf::RectangleShape outline(bounds.size);
                outline.setPosition(bounds.position);
                outline.setFillColor(sf::Color::Transparent);
                outline.setOutlineThickness(2.f);
                outline.setOutlineColor(debugColors[colorOffset + place]);
                win_.draw(outline);
            }
        }
    };

    drawColumn(GameMode::Classic, 357.f, 0);
    drawColumn(GameMode::Endless, 770.f, 3);
}

/**
 * @brief Performs the render ranking operation while preserving the current UI state invariants.
 */
void UIManager::renderRanking()
{
    constexpr std::array<float, 3> rowY = { 286.f, 410.f, 536.f };
    constexpr std::array<sf::Color, 6> debugColors = {
        sf::Color::Red, sf::Color::Green, sf::Color::Blue,
        sf::Color::Yellow, sf::Color::Magenta, sf::Color::Cyan
    };
    auto drawColumn = [&](GameMode mode, float x, int colorOffset) {
        const std::vector<RunRecord> rows = ranks_.all(mode);
        for (int place = 0; place < 3; ++place) {
            const bool hasRecord = place < static_cast<int>(rows.size());
            const std::string name = hasRecord ? rows[place].name : "---";
            const std::string time = hasRecord ? std::to_string(rows[place].elapsedSec) + "s" : "--";
            sf::Text entry(rankingFont_, name + "\nTime: " + time, 22);
            entry.setFillColor(sf::Color::White);
            entry.setPosition({ x, rowY[place] });
            win_.draw(entry);
            if (debugUi_) {
                const sf::FloatRect bounds = entry.getGlobalBounds();
                sf::RectangleShape outline(bounds.size);
                outline.setPosition(bounds.position);
                outline.setFillColor(sf::Color::Transparent);
                outline.setOutlineThickness(2.f);
                outline.setOutlineColor(debugColors[colorOffset + place]);
                win_.draw(outline);
            }
        }
    };
    drawColumn(GameMode::Classic, 350.f, 0);
    drawColumn(GameMode::Endless, 758.f, 3);
}


/**
 * @brief Performs the render help operation while preserving the current UI state invariants.
 */
void UIManager::renderHelp()
{
    drawCenteredText("HOW TO PLAY", 80, 40, sf::Color::White, true);
    const std::string lines[] = {
        "- Classic: clear all 10 levels in order.",
        "- Endless: survive as long as possible.",
        "- New Game: pick a mode and a name to start.",
        "- Load Game: continue from a saved slot (3 per mode).",
        "- Ranking: see the top 100 entries, scrolled 10 at a time.",
        "- Setting: adjust audio volume.",
        "- Graphic: pick a character and a background.",
    };
    for (int i = 0; i < 7; ++i)
    {
        sf::Text t(font_, lines[i], 22);
        t.setFillColor(sf::Color(220, 220, 220));
        t.setPosition({ 200, 160.f + i * 40.f });
        win_.draw(t);
    }
    for (auto& b : btns_) b.draw(win_, font_);
}

/**
 * @brief Renders a map block with the camera offset while collision continues to use stable world coordinates.
 */
void UIManager::drawMapBlock(const MapBlock& block, float cameraY)
{

    mapBackground_.drawBlock(win_, mapImageKey(block), block.blockID,
                             block.startY, block.height(), cameraY);
}

/**
 * @brief Uses the block-selected artwork key to keep rendered lanes aligned with the generated map data.
 */
std::string UIManager::mapImageKey(const MapBlock& block) const
{

    if (!block.mapImageKey.empty())
        return block.mapImageKey;

    if (state_ == UIState::ClassicPlay) {
        const int level = std::clamp(ctx_.level, 1, CLASSIC_LEVELS);
        return "map_level_" + std::to_string(level) + (block.blockID == 0 ? "" : ".1");
    }
    return "map_level_" + std::to_string((block.blockID % CLASSIC_LEVELS) + 1);
}

/**
 * @brief Performs the draw player operation while preserving the current UI state invariants.
 */
void UIManager::drawPlayer()
{
    player_.draw(win_, cameraY_);
}

/**
 * @brief Draws gameplay in layer order so world actors remain behind HUD and sidebar controls.
 */
void UIManager::renderPlay()
{
    if (state_ == UIState::EndlessPlay) {
        for (const MapBlock& block : endlessMap_.getBlocks())
            drawMapBlock(block, cameraY_);
    }
    else {
        for (const MapBlock& block : classicMap_.getBlocks())
            drawMapBlock(block, cameraY_);
    }

    for (auto obs : Obstacles) {
        obs->draw(win_, cameraY_);
    }

    drawPlayer();

    // Draw the signal above world actors so its state remains readable without obscuring sidebar controls.
    win_.draw(trafficLightSprite_);

    if (debugUi_) {

        for (auto obs : Obstacles) {
            sf::FloatRect bounds = obs->getBounds();
            sf::RectangleShape box(bounds.size);
            box.setPosition({ bounds.position.x, bounds.position.y - cameraY_ });
            box.setFillColor(sf::Color::Transparent);
            box.setOutlineColor(sf::Color::Magenta);
            box.setOutlineThickness(-4.0f);
            win_.draw(box);
        }

        sf::FloatRect pBounds = player_.getBounds();
        sf::RectangleShape pBox(pBounds.size);
        pBox.setPosition({ pBounds.position.x, pBounds.position.y - cameraY_ });
        pBox.setFillColor(sf::Color::Transparent);
        pBox.setOutlineColor(sf::Color::Cyan);
        pBox.setOutlineThickness(-4.0f);
        win_.draw(pBox);
    }

    sf::RectangleShape sidebarBackground({ Grid::SIDEBAR_WIDTH, Grid::MAP_HEIGHT });
    sidebarBackground.setPosition({ Grid::MAP_WIDTH, 0.f });
    sidebarBackground.setFillColor(sf::Color(211, 211, 211));
    win_.draw(sidebarBackground);

    if (scoreTableAssetLoaded_ && scoreTableTex_.getSize().x != 0 && scoreTableTex_.getSize().y != 0) {
        sf::Sprite sidebar(scoreTableTex_);
        sidebar.setScale({ Grid::SIDEBAR_WIDTH / scoreTableTex_.getSize().x,
                           Grid::MAP_HEIGHT / scoreTableTex_.getSize().y });
        sidebar.setPosition({ Grid::MAP_WIDTH, 0.f });
        win_.draw(sidebar);
    }

    const int wholeSeconds = static_cast<int>(elapsedPlaySec_);
    const int minutes = wholeSeconds / 60;
    const int seconds = wholeSeconds % 60;
    const std::string timer = std::to_string(minutes / 10) +
        std::to_string(minutes % 10) + ":" +
        std::to_string(seconds / 10) + std::to_string(seconds % 10);

    const float scoreScaleY = Grid::MAP_HEIGHT / 1012.f;
    sf::Text timerText(font_, timer, static_cast<unsigned int>(48.f * scoreScaleY));
    timerText.setFillColor(sf::Color::White);
    const sf::FloatRect timerBounds = timerText.getLocalBounds();
    timerText.setPosition({ Grid::MAP_WIDTH + Grid::SIDEBAR_WIDTH * .5f -
                            (timerBounds.position.x + timerBounds.size.x * .5f),
                            285.f * scoreScaleY - timerBounds.position.y });
    win_.draw(timerText);

    auto drawShields = [&](const auto& mapBlocks) {
        for (const auto& block : mapBlocks) {
            for (int row = 0; row < LANES_PER_BLOCK; ++row) {
                if (block.shieldCols[row] != -1) {
                    sf::Sprite s(texShieldItem_);

                    s.setOrigin({ static_cast<float>(texShieldItem_.getSize().x) / 2.f, static_cast<float>(texShieldItem_.getSize().y) / 2.f });
                    s.setPosition({ Grid::columnCenter(block.shieldCols[row]), Grid::rowCenter(block.startY, row) - cameraY_ });
                    s.setScale({ 1.5f, 1.5f });
                    win_.draw(s);
                }
            }
        }
        };

    if (state_ == UIState::EndlessPlay) drawShields(endlessMap_.getBlocks());
    else drawShields(classicMap_.getBlocks());

    if (player_.hasShield()) {
        sf::Sprite hudShield(texShieldItem_);
        hudShield.setScale({ 1.5f, 1.5f });
        hudShield.setPosition({ 20.f, 20.f });
        win_.draw(hudShield);
    }

}

/**
 * @brief Performs the render game over operation while preserving the current UI state invariants.
 */
void UIManager::renderGameOver()
{
    // Win.png/Lose.png already contains the complete result presentation.
}

/**
 * @brief Performs the render pause operation while preserving the current UI state invariants.
 */
void UIManager::renderPause()
{
    if (!pauseAssetLoaded_ || pauseTex_.getSize().x == 0 || pauseTex_.getSize().y == 0)
        return;

    win_.setView(win_.getDefaultView());
    const sf::FloatRect bounds = pauseOverlayBounds();
    sf::Sprite overlay(pauseTex_);
    const sf::Vector2u textureSize = pauseTex_.getSize();
    overlay.setScale({ bounds.size.x / textureSize.x, bounds.size.y / textureSize.y });
    overlay.setPosition(bounds.position);
    win_.draw(overlay);
    win_.setView(uiView_);
}
