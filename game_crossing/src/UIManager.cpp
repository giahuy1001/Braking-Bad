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
#include <functional>
#include <iostream>
#include <limits>

// ---------------------------------------------------------------------
//  UIManager implementation.  Owns the entire UI/state machine; the old
//  GameState.h skeleton is gone.  Gameplay states (ClassicPlay/EndlessPlay/
//  Pause/GameOver) render placeholder labels so the project still builds
//  and runs end-to-end — the gameplay team plugs in the real logic later.
// ---------------------------------------------------------------------

namespace
{
    constexpr unsigned int  WINDOW_W   = 1920;
    constexpr unsigned int  WINDOW_H   = 1080;
    constexpr float         UI_SCALE   = 1.5f;
    constexpr float         UI_W       = WINDOW_W / UI_SCALE;
    constexpr float         UI_H       = WINDOW_H / UI_SCALE;
    constexpr float         BOOT_TIME  = 2.0f;
    constexpr float         PADDING    = 24.f;
    constexpr float         BTN_W      = 360.f;
    constexpr float         BTN_H      = 56.f;
    constexpr float         BTN_GAP    = 16.f;
    constexpr float         BACK_SIZE  = 40.f;
    constexpr float         BACK_X     = UI_W - PADDING - BACK_SIZE;
    constexpr float         BACK_Y     = PADDING;
    constexpr int           NAME_MAX   = 16;
    constexpr int           CLASSIC_LEVELS = 10;
    constexpr float         ENDLESS_SCROLL_SPEED = 180.f; // px/s, world Y decreases
    constexpr float         ENDLESS_CATCHUP_SPEED = 1500.f;
    constexpr float         CLASSIC_FOLLOW_SPEED = 960.f;  // px/s, catches up to player
    constexpr float         PLAYER_SCREEN_ANCHOR = Grid::MAP_HEIGHT * 0.65f;
    constexpr float         PLAYER_TOP_SAFE_LINE = 180.f;
    constexpr float         ENDLESS_CATCHUP_DISTANCE = Grid::CELL_SIZE * 4.f;

    sf::Color colorFromHex(unsigned int rgb)
    {
        return { static_cast<std::uint8_t>((rgb >> 16) & 0xFF),
                 static_cast<std::uint8_t>((rgb >>  8) & 0xFF),
                 static_cast<std::uint8_t>( rgb        & 0xFF) };
    }

    sf::Color laneColor(LaneType lane, BiomeType biome)
    {
        switch (lane) {
        case LaneType::Safe:
            return colorFromHex(0x4E8063); // Ép toàn bộ làn S thành màu xanh lá
        case LaneType::Vehicle:
            return colorFromHex(0x4A4E57); // Giữ nguyên màu xám
        case LaneType::Animal:
            return colorFromHex(0x8A704E); // Giữ nguyên màu vàng đất
        }
        return sf::Color::Magenta;
    }

    std::int64_t nowUnix()
    {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
}

const std::array<sf::FloatRect, 7> UIManager::kMainMenuButtonBounds = {
    // Right-side vertical menu: Play, Load, Graphic, Setting.
    sf::FloatRect({ 1416.f, 410.f }, { 314.f, 105.f }),
    sf::FloatRect({ 1416.f, 534.f }, { 314.f, 105.f }),
    sf::FloatRect({ 1416.f, 656.f }, { 314.f, 105.f }),
    sf::FloatRect({ 1416.f, 781.f }, { 314.f, 105.f }),
    // Bottom-right round icons: Leaderboard, Help, Exit.
    sf::FloatRect({ 1555.f, 980.f }, { 94.f, 94.f }),
    sf::FloatRect({ 1684.f, 980.f }, { 94.f, 94.f }),
    sf::FloatRect({ 1807.f, 980.f }, { 94.f, 94.f })
};

const std::array<std::string, 4> UIManager::kThemeNames = { "spring", "summer", "autumn", "winter" };
const sf::FloatRect UIManager::kCharacterPanelBounds({107.f, 409.f}, {351.f, 347.f});
const sf::FloatRect UIManager::kCharacterPrevBounds ({89.f, 562.f}, {43.f, 43.f});
const sf::FloatRect UIManager::kCharacterNextBounds ({433.f, 562.f}, {43.f, 43.f});
const sf::FloatRect UIManager::kThemePanelBounds    ({1414.f, 409.f}, {351.f, 347.f});
const sf::FloatRect UIManager::kThemePrevBounds     ({1396.f, 562.f}, {43.f, 43.f});
const sf::FloatRect UIManager::kThemeNextBounds     ({1740.f, 562.f}, {43.f, 43.f});
const sf::FloatRect UIManager::kSfxTrackBounds      ({1200.f, 380.f}, {400.f, 20.f});
const sf::FloatRect UIManager::kSfxDecBounds        ({1130.f, 365.f}, {50.f, 50.f});
const sf::FloatRect UIManager::kSfxIncBounds        ({1620.f, 365.f}, {50.f, 50.f});
const sf::FloatRect UIManager::kMusicTrackBounds    ({1200.f, 480.f}, {400.f, 20.f});
const sf::FloatRect UIManager::kMusicDecBounds      ({1130.f, 465.f}, {50.f, 50.f});
const sf::FloatRect UIManager::kMusicIncBounds      ({1620.f, 465.f}, {50.f, 50.f});
const sf::FloatRect UIManager::kSettingOkBounds     ({1390.f, 645.f}, {390.f, 86.f});

//Constructor
UIManager::UIManager(sf::RenderWindow& window)
    : win_(window),
    uiView_({ UI_W * 0.5f, UI_H * 0.5f }, { UI_W, UI_H }),
    debugText_(font_, "", 18),
    trafficLightSprite_(texTrafficGreen_) 
{
    // Use a project font first, then Windows fonts. This keeps debug text
    // visible even when the game is launched outside the IDE's working dir.
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

    debugText_.setCharacterSize(18);
    debugText_.setFillColor(sf::Color::Yellow);
    debugText_.setOutlineColor(sf::Color::Black);
    debugText_.setOutlineThickness(2.f);

    const bool lg  = logoTex_.loadFromFile("../Graphic/1x/DataList.png");
    (void)        iconsTex_.loadFromFile("../Graphic/1x/DataList.png");
    (void)lg; // The seasonal backgrounds are the only required UI images.

    bool tGreen = texTrafficGreen_.loadFromFile("assets/props/traffic_green.png");
    bool tYellow = texTrafficYellow_.loadFromFile("assets/props/traffic_yellow.png");
    bool tRed = texTrafficRed_.loadFromFile("assets/props/traffic_red.png");
    (void)tGreen; (void)tYellow; (void)tRed;

    trafficLightSprite_.setTexture(texTrafficGreen_, true);
    // Đặt tâm (origin) vào chính giữa bức ảnh
    sf::FloatRect bounds = trafficLightSprite_.getLocalBounds();
    trafficLightSprite_.setOrigin({ bounds.size.x / 2.f, 0.f });

    // Cột 6 (index 5) và Hàng 9 đếm từ dưới lên (tâm của ô trên cùng)
    trafficLightSprite_.setPosition({ Grid::columnCenter(5), 0.f });

    setTheme("spring");

    cfg_ = sets_.load();
    cfg_.volume = std::clamp(cfg_.volume, 0, 100);
    cfg_.musicVolume = std::clamp(cfg_.musicVolume, 0, 100);
    applyAudioVolumes();
    cfg_.cosmetic.characterId = std::clamp(cfg_.cosmetic.characterId, 1,
                                           CharacterRenderer::kCharacterCount);
    ctx_.selectedCharacterID = cfg_.cosmetic.characterId;
    saves_.loadAll();
    ranks_.loadAll();
    prog_.load();

    setState(UIState::Boot);

    // AudioManager tu load va phan loai asset. applyAudioVolumes() da gui
    // volume trong Settings vao manager truoc do, nen manager se ap dung no
    // ngay sau khi cac asset duoc nap.
    audio_.loadAssets();
}

UIManager::~UIManager() = default;

bool UIManager::setTheme(const std::string& seasonName)
{
    if (seasonName.empty()) return false;

    // Asset folders in this project currently start with a capital letter,
    // while the public API accepts the documented lowercase names. Try both.
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

    sf::Texture newMain, newSetting, newGraphic;
    if (!load(newMain, "MainMenu", true) || !load(newSetting, "Setting", false) ||
        !load(newGraphic, "Graphic", false))
    {
        std::cerr << "[UIManager] failed to load theme '" << seasonName
                  << "' from assets/theme/<season>/\n";
        return false;
    }

    bgTex_ = std::move(newMain);
    settingBgTex_ = std::move(newSetting);
    graphicBgTex_ = std::move(newGraphic);
    currentTheme_ = seasonName;
    assetsLoaded_ = true;
    return true;
}

// ---------------------------------------------------------------------
//  Public run loop
// ---------------------------------------------------------------------
void UIManager::run()
{
    sf::Clock frameClock;
    while (win_.isOpen())
    {
        handleEvents();
        update(frameClock.restart().asSeconds());
        render();
    }
}

// ---------------------------------------------------------------------
//  Event handling
// ---------------------------------------------------------------------
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

            // Khi mixer dang mo, cac phim dieu khien no khong duoc roi xuong
            // gameplay (vi du Up/Down se khong lam Player di chuyen).
            if (debugAudioMixer_ && handleDebugAudioMixerKey(*key))
                continue;

            if (key->code == sf::Keyboard::Key::F3 || key->code == sf::Keyboard::Key::D)
            {
                debugUi_ = !debugUi_;
                std::cout << "[UI Debug] " << (debugUi_ ? "enabled" : "disabled") << '\n';
                continue;
            }
        }

        // Debug clicks are intentionally consumed: measuring a coordinate must
        // not accidentally start a game or close the application.
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

        // Modal always wins while it is up.
        if (modal_ != Modal::None)
        {
            handleModal(*event);
            continue;
        }

        switch (state_)
        {
        case UIState::Boot:         handleBoot(*event);     break;
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

// ---------------------------------------------------------------------
//  Per-state event handlers
// ---------------------------------------------------------------------
void UIManager::handleBoot(const sf::Event& e)
{
    if (e.is<sf::Event::KeyPressed>())
        setState(UIState::MainMenu);
}

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
            if (button >= 0) activateMainMenuButton(static_cast<std::size_t>(button));
        }
    }
    if (e.is<sf::Event::MouseMoved>())
    {
        const auto* mm = e.getIf<sf::Event::MouseMoved>();
        const int button = menuButtonAt(mm->position);
        if (button >= 0) focusIdx_ = button;
    }
}

void UIManager::handleModeSel(const sf::Event& e)
{
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape || k->code == sf::Keyboard::Key::Backspace) { handleBack(); return; }
        if (k->code == sf::Keyboard::Key::Enter) { activateFocused(); return; }
        if (k->code == sf::Keyboard::Key::Up)   { moveFocus(-1); return; }
        if (k->code == sf::Keyboard::Key::Down) { moveFocus(+1); return; }
    }
    if (const auto* mm = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mm->button == sf::Mouse::Button::Left)
        {
            sf::Vector2f mp(static_cast<float>(mm->position.x),
                            static_cast<float>(mm->position.y));
            for (int i = 0; i < (int)btns_.size(); ++i)
            {
                if (btns_[i].consumeClick(mp))
                {
                    focusIdx_ = i;
                    activateFocused();
                    return;
                }
            }
        }
    }
    if (e.is<sf::Event::MouseMoved>())
    {
        const auto* mm = e.getIf<sf::Event::MouseMoved>();
        sf::Vector2f mp(static_cast<float>(mm->position.x),
                        static_cast<float>(mm->position.y));
        for (int i = 0; i < (int)btns_.size(); ++i)
        {
            btns_[i].update(mp);
            if (btns_[i].contains(mp)) focusIdx_ = i;
        }
    }
}

void UIManager::handleName(const sf::Event& e)
{
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape)        { handleBack(); return; }
        if (k->code == sf::Keyboard::Key::Backspace)    { if (!nameBuffer_.empty()) nameBuffer_.pop_back(); return; }
        if (k->code == sf::Keyboard::Key::Enter)        { if (isValidName(nameBuffer_)) { ctx_.pendingName = nameBuffer_; if (ctx_.mode == StateContext::Mode::Classic) setState(UIState::LevelSelect); else setState(UIState::EndlessPlay); } return; }
    }
    if (const auto* t = e.getIf<sf::Event::TextEntered>())
    {
        if (isValidNameChar(static_cast<std::uint32_t>(t->unicode)) && (int)nameBuffer_.size() < NAME_MAX)
        {
            nameBuffer_ += static_cast<char>(t->unicode);
        }
    }
}

void UIManager::handleLvlSel(const sf::Event& e)
{
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape || k->code == sf::Keyboard::Key::Backspace) { handleBack(); return; }
        if (k->code == sf::Keyboard::Key::Enter) { activateFocused(); return; }
        if (k->code == sf::Keyboard::Key::Left)  { moveFocus(-1); return; }
        if (k->code == sf::Keyboard::Key::Right) { moveFocus(+1); return; }
        if (k->code == sf::Keyboard::Key::Up)    { moveFocus(-5); return; }
        if (k->code == sf::Keyboard::Key::Down)  { moveFocus(+5); return; }
    }
    if (const auto* mm = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mm->button == sf::Mouse::Button::Left)
        {
            sf::Vector2f mp(static_cast<float>(mm->position.x),
                            static_cast<float>(mm->position.y));
            for (int i = 0; i < (int)btns_.size(); ++i)
            {
                if (btns_[i].consumeClick(mp))
                {
                    focusIdx_ = i;
                    activateFocused();
                    return;
                }
            }
        }
    }
    if (e.is<sf::Event::MouseMoved>())
    {
        const auto* mm = e.getIf<sf::Event::MouseMoved>();
        sf::Vector2f mp(static_cast<float>(mm->position.x),
                        static_cast<float>(mm->position.y));
        for (int i = 0; i < (int)btns_.size(); ++i)
        {
            btns_[i].update(mp);
            if (btns_[i].contains(mp)) focusIdx_ = i;
        }
    }
}

void UIManager::handleSetting(const sf::Event& e)
{
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape) { handleBack(); return; }
        if (k->code == sf::Keyboard::Key::Left)  { cfg_.volume = std::max(0, cfg_.volume - 5); applyAudioVolumes(); return; }
        if (k->code == sf::Keyboard::Key::Right) { cfg_.volume = std::min(100, cfg_.volume + 5); applyAudioVolumes(); return; }
    }
    if (const auto* mm = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mm->button == sf::Mouse::Button::Left)
        {
            const sf::Vector2f p(mm->position.x, mm->position.y);
            if (scaledBaseRect(kSettingOkBounds).contains(p)) { sets_.save(cfg_); setState(UIState::MainMenu); return; }
            if (scaledBaseRect(kSfxDecBounds).contains(p)) { cfg_.volume = std::max(0, cfg_.volume - 5); applyAudioVolumes(); return; }
            if (scaledBaseRect(kSfxIncBounds).contains(p)) { cfg_.volume = std::min(100, cfg_.volume + 5); applyAudioVolumes(); return; }
            if (scaledBaseRect(kMusicDecBounds).contains(p)) { cfg_.musicVolume = std::max(0, cfg_.musicVolume - 5); applyAudioVolumes(); return; }
            if (scaledBaseRect(kMusicIncBounds).contains(p)) { cfg_.musicVolume = std::min(100, cfg_.musicVolume + 5); applyAudioVolumes(); return; }
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

void UIManager::handleGraphic(const sf::Event& e)
{
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape) { handleBack(); return; }
        if (k->code == sf::Keyboard::Key::Left)
        {
            cfg_.cosmetic.characterId = CharacterRenderer::normalizeID(cfg_.cosmetic.characterId - 1);
            ctx_.selectedCharacterID = cfg_.cosmetic.characterId;
            return;
        }
        if (k->code == sf::Keyboard::Key::Right)
        {
            cfg_.cosmetic.characterId = CharacterRenderer::normalizeID(cfg_.cosmetic.characterId + 1);
            ctx_.selectedCharacterID = cfg_.cosmetic.characterId;
            return;
        }
    }
    if (const auto* mm = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mm->button == sf::Mouse::Button::Left)
        {
            const sf::Vector2f p(mm->position.x, mm->position.y);
            if (scaledBaseRect(kCharacterPrevBounds).contains(p)) { cfg_.cosmetic.characterId = CharacterRenderer::normalizeID(cfg_.cosmetic.characterId - 1); ctx_.selectedCharacterID = cfg_.cosmetic.characterId; return; }
            if (scaledBaseRect(kCharacterNextBounds).contains(p)) { cfg_.cosmetic.characterId = CharacterRenderer::normalizeID(cfg_.cosmetic.characterId + 1); ctx_.selectedCharacterID = cfg_.cosmetic.characterId; return; }
            if (scaledBaseRect(kThemePrevBounds).contains(p)) { currentThemeIndex_ = (currentThemeIndex_ + 3) % 4; setTheme(kThemeNames[currentThemeIndex_]); return; }
            if (scaledBaseRect(kThemeNextBounds).contains(p)) { currentThemeIndex_ = (currentThemeIndex_ + 1) % 4; setTheme(kThemeNames[currentThemeIndex_]); return; }
        }
    }
}

void UIManager::handleLoad(const sf::Event& e)
{
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape)        { handleBack(); return; }
        if (k->code == sf::Keyboard::Key::Backspace)     { handleBack(); return; }
        if (k->code == sf::Keyboard::Key::Enter)         { activateFocused(); return; }
        if (k->code == sf::Keyboard::Key::Up)            { moveFocus(-3); return; }
        if (k->code == sf::Keyboard::Key::Down)          { moveFocus(+3); return; }
        if (k->code == sf::Keyboard::Key::Left)          { moveFocus(-1); return; }
        if (k->code == sf::Keyboard::Key::Right)         { moveFocus(+1); return; }
        if (k->code == sf::Keyboard::Key::Tab)           { loadTabModeIdx_ = 1 - loadTabModeIdx_; rebuildButtons(); return; }
        if (k->code == sf::Keyboard::Key::Delete)
        {
            const int idx = focusIdx_;
            if (idx >= 0 && idx < (int)SaveStore::kMaxSlots)
            {
                saves_.clear(idx, loadTabModeIdx_ == 0 ? GameMode::Classic : GameMode::Endless);
                rebuildButtons();
            }
            return;
        }
    }
    if (const auto* mm = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mm->button == sf::Mouse::Button::Left)
        {
            sf::Vector2f mp(static_cast<float>(mm->position.x),
                            static_cast<float>(mm->position.y));
            // Tab buttons are the first two entries in btns_ on the Load screen.
            if (!btns_.empty() && btns_[0].contains(mp)) { loadTabModeIdx_ = 0; rebuildButtons(); return; }
            if (btns_.size() > 1 && btns_[1].contains(mp)) { loadTabModeIdx_ = 1; rebuildButtons(); return; }
            for (size_t i = 2; i < btns_.size(); ++i)
            {
                if (btns_[i].consumeClick(mp)) { focusIdx_ = (int)i; activateFocused(); return; }
            }
        }
    }
    if (e.is<sf::Event::MouseMoved>())
    {
        const auto* mm = e.getIf<sf::Event::MouseMoved>();
        sf::Vector2f mp(static_cast<float>(mm->position.x),
                        static_cast<float>(mm->position.y));
        for (int i = 0; i < (int)btns_.size(); ++i)
        {
            btns_[i].update(mp);
            if (btns_[i].contains(mp)) focusIdx_ = i;
        }
    }
}

void UIManager::handleRanking(const sf::Event& e)
{
    if (const auto* w = e.getIf<sf::Event::MouseWheelScrolled>())
    {
        rankScrollOffset_ += (w->delta > 0 ? -1 : 1);
        return;
    }
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape)  { handleBack(); return; }
        if (k->code == sf::Keyboard::Key::Up)     { rankScrollOffset_ = std::max(0, rankScrollOffset_ - 1); return; }
        if (k->code == sf::Keyboard::Key::Down)   { rankScrollOffset_ += 1; return; }
        if (k->code == sf::Keyboard::Key::PageUp) { rankScrollOffset_ = std::max(0, rankScrollOffset_ - 5); return; }
        if (k->code == sf::Keyboard::Key::PageDown) { rankScrollOffset_ += 5; return; }
        if (k->code == sf::Keyboard::Key::Tab)    { rankTabModeIdx_ = 1 - rankTabModeIdx_; rankScrollOffset_ = 0; rebuildButtons(); return; }
    }
    if (const auto* mm = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mm->button == sf::Mouse::Button::Left)
        {
            sf::Vector2f mp(static_cast<float>(mm->position.x),
                            static_cast<float>(mm->position.y));
            if (!btns_.empty() && btns_[0].contains(mp)) { rankTabModeIdx_ = 0; rankScrollOffset_ = 0; rebuildButtons(); return; }
            if (btns_.size() > 1 && btns_[1].contains(mp)) { rankTabModeIdx_ = 1; rankScrollOffset_ = 0; rebuildButtons(); return; }
        }
    }
    if (e.is<sf::Event::MouseMoved>())
    {
        const auto* mm = e.getIf<sf::Event::MouseMoved>();
        sf::Vector2f mp(static_cast<float>(mm->position.x),
                        static_cast<float>(mm->position.y));
        for (int i = 0; i < (int)btns_.size(); ++i)
        {
            btns_[i].update(mp);
            if (btns_[i].contains(mp)) focusIdx_ = i;
        }
    }
}

void UIManager::handleHelp(const sf::Event& e)
{
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape || k->code == sf::Keyboard::Key::Enter)
            handleBack();
    }
    if (const auto* mm = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mm->button == sf::Mouse::Button::Left) handleBack();
    }
}

void UIManager::handlePlay(const sf::Event& e)
{
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

        // Once the player is fully below the viewport, ignore every movement
        // key. This prevents clamping an off-screen Y back onto a visible row.
        if (state_ == UIState::EndlessPlay && isEndlessPlayerOffscreen())
        {
            finishEndlessRun();
            return;
        }

        // The first directional input starts both clock and camera.
        gameplayStarted_ = true;

        const float topLimit = state_ == UIState::ClassicPlay
            ? classicMap_.topLimit() + Grid::CELL_SIZE * 0.5f
            : std::numeric_limits<float>::lowest() / 4.f;
        const float mapBottom = state_ == UIState::ClassicPlay
            ? classicMap_.bottomLimit() - Grid::CELL_SIZE * 0.5f
            : -Grid::CELL_SIZE * 0.5f;
        // maxWalkablePlayerY() only returns a row that is 100% visible. It
        // therefore prevents stepping into a partially clipped bottom tile.
        const float minY = topLimit;
        const float maxY = std::min(mapBottom, maxWalkablePlayerY());
        // CPlayer owns grid stepping, horizontal clamping, and rejection of
        // invalid vertical moves. Its bounds retain the exact map/view math.
        player_.setMovementBounds({ { Grid::playableLeftCenter(), minY },
                                    { Grid::playableRightCenter() - Grid::playableLeftCenter(),
                                      maxY - minY } });
        player_.handleInput(k->code);
        const sf::Vector2f playerPosition = player_.getPosition();

        // ĐOẠN CODE CẦN THÊM: Kiểm tra xem người chơi có đạp trúng nắp cống không
        int playerCol = static_cast<int>(playerPosition.x / Grid::CELL_SIZE);
        bool steppedOnManhole = false;

        auto checkManhole = [&](const auto& mapBlocks) {
            for (const auto& block : mapBlocks) {
                // Kiểm tra xem người chơi đang đứng trong block (bản đồ) nào
                if (block.contains(playerPosition.y)) {
                    // Tính toán xem đang đứng ở hàng (row) thứ mấy trong block đó
                    int row = static_cast<int>((playerPosition.y - block.startY) / Grid::CELL_SIZE);
                    if (row >= 0 && row < LANES_PER_BLOCK) {
                        // Nếu vị trí cột của người chơi trùng với cột có nắp cống (-1 là không có)
                        if (block.manholeCols[row] == playerCol) {
                            steppedOnManhole = true;
                        }
                    }
                    break; // Đã tìm thấy block hiện tại nên không cần duyệt tiếp
                }
            }
            };

        // Áp dụng kiểm tra cho chế độ tương ứng
        if (state_ == UIState::ClassicPlay) {
            checkManhole(classicMap_.getBlocks());
        }
        else if (state_ == UIState::EndlessPlay) {
            checkManhole(endlessMap_.getBlocks());
        }

        // Xử lý GameOver nếu đạp trúng nắp cống
        if (steppedOnManhole) {
            player_.kill();
            if (state_ == UIState::EndlessPlay) {
                finishEndlessRun();
            }
            else {
                classicWon_ = false; // Đánh dấu thua game ở chế độ Classic
                ctx_.classicLevel = ctx_.level;
                ctx_.classicSec = static_cast<int>(elapsedPlaySec_);
                setState(UIState::GameOver);
            }
            return; // Dừng hàm tại đây để không cập nhật thêm logic chiến thắng
        }

        // The centre of the final block's top row is the Classic finish tile.
        if (state_ == UIState::ClassicPlay &&
            player_.getPosition().y <= classicMap_.topLimit() + Grid::CELL_SIZE * 0.5f)
        {
            classicWon_ = true;
            ctx_.classicLevel = ctx_.level;
            ctx_.classicSec = static_cast<int>(elapsedPlaySec_);
            prog_.setHighestUnlockedLevel(std::max(prog_.highestUnlockedLevel(),
                                                   std::min(CLASSIC_LEVELS, ctx_.level + 1)));
            setState(UIState::GameOver);
        }
    }
}

void UIManager::handleGameOver(const sf::Event& e)
{
    // 1. ADD STANDARD UI BUTTON INTERACTION (Mouse Clicks & Navigation)
    if (const auto* m = e.getIf<sf::Event::MouseButtonPressed>()) {
        if (m->button == sf::Mouse::Button::Left) {
            int clicked = menuButtonAt(sf::Vector2i(m->position.x, m->position.y));
            if (clicked >= 0) {
                focusIdx_ = clicked;
                activateFocused(); // Activates the clicked button
                return;
            }
        }
    }
    else if (const auto* k = e.getIf<sf::Event::KeyPressed>()) {
        if (k->code == sf::Keyboard::Key::Up || k->code == sf::Keyboard::Key::W) {
            moveFocus(-1);
            return;
        }
        else if (k->code == sf::Keyboard::Key::Down || k->code == sf::Keyboard::Key::S) {
            moveFocus(1);
            return;
        }
        else if (k->code == sf::Keyboard::Key::Enter) {
            // If we have actual UI buttons on screen, pressing Enter should activate them!
            if (!btns_.empty()) {
                activateFocused();
                return;
            }
        }
    }

    // 2. YOUR ORIGINAL FALLBACK RANKING LOGIC
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Enter || k->code == sf::Keyboard::Key::R)
        {
            if (classicWon_)
            {
                RunRecord r;
                r.name = ctx_.pendingName;
                r.mode = GameMode::Classic;
                r.level = ctx_.classicLevel;
                r.elapsedSec = ctx_.classicSec;
                r.score = 0;
                r.savedAtUnix = nowUnix();
                ranks_.submit(r);

                classicWon_ = false;
                setState(UIState::LevelSelect);
                return;
            }

            RunRecord r;
            r.name = ctx_.pendingName;
            if (ctx_.mode == StateContext::Mode::Classic)
            {
                r.mode = GameMode::Classic;
                r.level = ctx_.classicLevel;
                r.elapsedSec = ctx_.classicSec;
                r.score = 0;
            }
            else
            {
                r.mode = GameMode::Endless;
                r.level = 0;
                r.elapsedSec = ctx_.endlessSec;
                r.score = ctx_.endlessScore;
            }
            r.savedAtUnix = nowUnix();
            ranks_.submit(r);
            handleBack();
        }
    }
}

void UIManager::handlePause(const sf::Event& e)
{
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape)
        {
            // TẠO BẢN LƯU TRƯỚC KHI THOÁT
            RunRecord r;
            r.name = ctx_.pendingName;

            if (ctx_.mode == StateContext::Mode::Classic) {
                r.mode = GameMode::Classic;
                r.level = ctx_.level;
                r.elapsedSec = static_cast<int>(elapsedPlaySec_);
                r.score = 0;
            }
            else {
                r.mode = GameMode::Endless;
                r.level = 0;
                r.elapsedSec = static_cast<int>(elapsedPlaySec_);
                r.score = ctx_.endlessScore;
            }
            r.savedAtUnix = nowUnix();

            // Lưu chính xác tọa độ
            r.playerX = player_.getPosition().x;
            r.playerY = player_.getPosition().y;
            r.cameraY = cameraY_;

            // Đẩy vào slot mới nhất
            saves_.push(r);

            handleBack();
        }
        if (k->code == sf::Keyboard::Key::Enter)
        {
            setState(ctx_.mode == StateContext::Mode::Endless
                ? UIState::EndlessPlay
                : UIState::ClassicPlay);
        }
    }
}

void UIManager::handleModal(const sf::Event& e)
{
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape) { modal_ = Modal::None; return; }
        if (k->code == sf::Keyboard::Key::Left || k->code == sf::Keyboard::Key::Right ||
            k->code == sf::Keyboard::Key::Up   || k->code == sf::Keyboard::Key::Down) { modalFocus_ = 1 - modalFocus_; return; }
        if (k->code == sf::Keyboard::Key::Enter)
        {
            if (modalFocus_ == 0) win_.close();
            else                  modal_ = Modal::None;
            return;
        }
    }
    if (const auto* mm = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mm->button == sf::Mouse::Button::Left)
        {
            sf::Vector2f mp = toUiCoords(mm->position);
            sf::FloatRect yes({ UI_W/2.f - 160, UI_H/2.f + 20 }, { 140, 50 });
            sf::FloatRect no ({ UI_W/2.f +  20, UI_H/2.f + 20 }, { 140, 50 });
            if (yes.contains(mp))  win_.close();
            if (no.contains(mp))   modal_ = Modal::None;
        }
    }
}

// ---------------------------------------------------------------------
//  Per-frame update
// ---------------------------------------------------------------------
void UIManager::update(float dt)
{
    if (state_ == UIState::Boot)
    {
        bootTimer_ += dt;
        if (bootTimer_ >= BOOT_TIME) setState(UIState::MainMenu);
    }

    if (state_ == UIState::Ranking)
    {
        const GameMode m = (rankTabModeIdx_ == 0) ? GameMode::Classic : GameMode::Endless;
        const auto rows = ranks_.all(m);
        const int maxOffset = std::max(0, (int)rows.size() - RankingStore::kMaxVisible);
        rankScrollOffset_ = std::clamp(rankScrollOffset_, 0, maxOffset);
    }

    // Resolve an already off-screen Endless player before advancing gameplay.
    // Event input is also guarded in handlePlay(), covering this frame's keys.
    if (gameplayStarted_ && state_ == UIState::EndlessPlay && isEndlessPlayerOffscreen())
    {
        finishEndlessRun();
        return;
    }

    if (gameplayStarted_ && (state_ == UIState::ClassicPlay || state_ == UIState::EndlessPlay))
    {
        // 1. Play or Resume traffic if actively playing
        if (!audio_.vehicleAmbiencePlaying()) {
            audio_.startVehicleAmbience();
        }

        elapsedPlaySec_ += dt;
        updateCamera(dt);
        player_.setCameraOffset(cameraY_);
        player_.update(dt);
    }

    else if (state_ == UIState::Pause || state_ != UIState::ClassicPlay && state_ != UIState::EndlessPlay)
    {
        // 2. Pause traffic when paused, or in menus like Game Over / Main Menu
        if (audio_.vehicleAmbiencePlaying()) {
            audio_.pauseVehicleAmbience();
        }
    }

    if (gameplayStarted_ && state_ == UIState::EndlessPlay)
    {
        endlessMap_.update(cameraY_);
        // World position remains unchanged; loss occurs only once the player
        // sprite has passed completely below the bottom view edge.
        if (isEndlessPlayerOffscreen())
            finishEndlessRun();
    }

    // Update obstacles only during active gameplay states
    if (state_ == UIState::ClassicPlay || state_ == UIState::EndlessPlay) {
        if (gameplayStarted_) {

            // Endless mode speed scaling
            float currentMultiplier = 1.0f;
            if (state_ == UIState::EndlessPlay) {
                float distanceTraveled = std::abs(player_.getPosition().y + 1080.0f);
                if (distanceTraveled > 15000.f) currentMultiplier = 2.0f;
                else if (distanceTraveled > 5000.f) currentMultiplier = 1.5f;
            }

            // 1. Move all obstacles first
            for (auto obs : Obstacles) {
                obs->move(dt);
            }

            // 2. Clean up off-screen obstacles using Swap-and-Pop (O(1) complexity!)
            for (int i = 0; i < Obstacles.size(); ) {
                if (Obstacles[i]->isOffScreen()) {
                    delete Obstacles[i]; // Free the memory

                    // Overwrite this slot with the last element in the vector
                    Obstacles[i] = Obstacles.back();
                    // Shrink the vector by 1
                    Obstacles.pop_back();

                    // Note: We DO NOT increment 'i' here, because we need to 
                    // check the new swapped-in element on the next loop!
                }
                else {
                    ++i;
                }
            }

            // 3. THE SPAWNER
            obstacleSpawnTimer_ -= dt;
            if (obstacleSpawnTimer_ <= 0.f) {
                obstacleSpawnTimer_ = 0.6f; // Spawn slightly faster for more traffic

                // Helper lambda to scan map blocks and spawn obstacles
                auto spawnInBlocks = [&](const auto& blocks) {
                    for (const MapBlock& block : blocks) {
                        for (int row = 0; row < LANES_PER_BLOCK; ++row) {
                            LaneType type = block.lanes[row];
                            if (type == LaneType::Safe) continue;

                            direction dir = ((block.blockID + row) % 2 == 0) ? RIGHT : LEFT;

                            if (rand() % 100 < 50) {
                                float laneTopY = block.startY + (row * Grid::CELL_SIZE);
                                float rowY = laneTopY + (Grid::CELL_SIZE * 0.25f);
                                float spawnX = (dir == RIGHT) ? Grid::GRID_LEFT - 150.f : Grid::GRID_RIGHT + 150.f;

                                bool isBlocked = false;
                                for (auto obs : Obstacles) {
                                    if (std::abs(obs->getY() - rowY) < 1.0f) {
                                        if (std::abs(obs->getX() - spawnX) < 400.f) {
                                            isBlocked = true;
                                            break;
                                        }
                                    }
                                }

                                if (!isBlocked) {
                                    if (type == LaneType::Vehicle) {
                                        if (row % 2 == 0) {
                                            Obstacles.push_back(new CCar(spawnX, rowY, dir));
                                        }
                                        else {
                                            Obstacles.push_back(new CTruck(spawnX, rowY, dir));
                                        }
                                    }
                                    else if (type == LaneType::Animal) {
                                        if (row % 2 == 0) {
                                            Obstacles.push_back(new CCat(spawnX, rowY, dir));
                                            // 10% chance for the cat to meow when it spawns!
                                            if (rand() % 100 < 10) audio_.playAnimalSample(false); //false = cat
                                        }
                                        else {
                                            Obstacles.push_back(new CDeer(spawnX, rowY, dir));
                                            // 5% chance for the deer to grunt when it spawns!
                                            if (rand() % 100 < 5) audio_.playAnimalSample(true); //true = deer
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
            } // <-- SPAWNER BLOCK ENDS HERE

            // --- 4. TRAFFIC LIGHT SYSTEM ---
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

            // Update vehicle behavior based on the current light state
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
            // -----------------------------------------------------------------------

            // --- 5. COLLISION DETECTION ---
            bool hit = false;
            for (auto obs : Obstacles) {
                if (CVehicle* v = dynamic_cast<CVehicle*>(obs)) {
                    if (player_.isImpact(v)) {
                        hit = true;
                        break;
                    }
                }
                else if (CAnimal* a = dynamic_cast<CAnimal*>(obs)) {
                    if (player_.isImpact(a)) {
                        hit = true;
                        break;
                    }
                }
            }

            if (hit) {
                player_.kill();
                audio_.playUiCrash();
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

// ---------------------------------------------------------------------
//  State management helpers
// ---------------------------------------------------------------------
void UIManager::setState(UIState s)
{
    // Returning from Pause must preserve the current camera and generated
    // blocks. All other transitions into a play state start a fresh run.
    const bool enteringClassic = s == UIState::ClassicPlay &&
                                 state_ != UIState::ClassicPlay && state_ != UIState::Pause;
    const bool enteringEndless = s == UIState::EndlessPlay &&
                                 state_ != UIState::EndlessPlay && state_ != UIState::Pause;

    state_  = s;
    if (enteringClassic || enteringEndless)
        resetGameplay();
    focusIdx_ = 0;
    rebuildButtons();
}

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

    // Clean up any old test obstacles
    for (auto obs : Obstacles) {
        delete obs;
    }
    Obstacles.clear();
    obstacleSpawnTimer_ = 0.f; // Reset the timer when a new game starts  
    currentLight_ = TrafficLight::Green;
    trafficLightTimer_ = 0.f;
    trafficLightSprite_.setTexture(texTrafficGreen_);
}

float UIManager::maxWalkablePlayerY() const
{
    // A complete row must have its bottom edge at or above the viewport's
    // lower edge. floor() correctly handles negative world coordinates.
    const float viewBottom = cameraY_ + Grid::MAP_HEIGHT;
    const float lastCompleteRowBottom = std::floor(viewBottom / Grid::CELL_SIZE) * Grid::CELL_SIZE;
    return lastCompleteRowBottom - Grid::CELL_SIZE * 0.5f;
}

bool UIManager::isEndlessPlayerOffscreen() const
{
    return player_.getPosition().y - cameraY_ - player_.getBounds().size.y * 0.5f > Grid::MAP_HEIGHT;
}

void UIManager::finishEndlessRun()
{
    if (state_ != UIState::EndlessPlay) return;
    player_.kill();
    // Capture result before any future resetGameplay()/EndlessMap::reset().
    ctx_.endlessSec = static_cast<int>(elapsedPlaySec_);
    setState(UIState::GameOver);
}

void UIManager::updateCamera(float dt)
{
    if (state_ == UIState::EndlessPlay)
    {
        // Base scroll moves up continuously after the first input.
        cameraY_ -= ENDLESS_SCROLL_SPEED * dt;

        // When a fast player approaches the top 3-4 rows, smoothly move the
        // camera farther up to restore the normal player anchor. This is a
        // catch-up, not a change to the player's world position.
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

    // Classic follows the advancing player only. At the top block, its start
    // becomes the camera's immutable stop: that final 1080px block is fully
    // visible and remains so for the rest of the level.
    const float stopY = classicMap_.topLimit();
    const float targetY = std::max(stopY, player_.getPosition().y - PLAYER_SCREEN_ANCHOR);
    if (targetY < cameraY_)
        cameraY_ = std::max(targetY, cameraY_ - CLASSIC_FOLLOW_SPEED * dt);

    // A rapid sequence of player moves cannot let the player leave through
    // the top while the smoothed camera is catching up.
    const float safeCameraY = std::max(stopY, player_.getPosition().y - PLAYER_TOP_SAFE_LINE);
    if (cameraY_ > safeCameraY)
        cameraY_ = safeCameraY;
}

sf::Vector2f UIManager::toUiCoords(sf::Vector2i pixel) const
{
    return win_.mapPixelToCoords(pixel, uiView_);
}

sf::Vector2f UIManager::toBaseCoords(sf::Vector2i pixel) const
{
    const sf::Vector2u size = win_.getSize();
    if (size.x == 0 || size.y == 0) return {};
    return { pixel.x * (WINDOW_W / static_cast<float>(size.x)),
             pixel.y * (WINDOW_H / static_cast<float>(size.y)) };
}

sf::FloatRect UIManager::scaledMenuBounds(std::size_t index) const
{
    return scaledBaseRect(kMainMenuButtonBounds.at(index));
}

sf::FloatRect UIManager::scaledBaseRect(const sf::FloatRect& base) const
{
    const sf::Vector2u size = win_.getSize();
    const float sx = size.x / static_cast<float>(WINDOW_W);
    const float sy = size.y / static_cast<float>(WINDOW_H);
    return { { base.position.x * sx, base.position.y * sy },
             { base.size.x * sx, base.size.y * sy } };
}

void UIManager::setSfxVolumeFromMouse(sf::Vector2i pixel)
{
    const sf::FloatRect track = scaledBaseRect(kSfxTrackBounds);
    const float ratio = (pixel.x - track.position.x) / track.size.x;
    cfg_.volume = std::clamp(static_cast<int>(std::lround(ratio * 100.f)), 0, 100);
    applyAudioVolumes();
}

void UIManager::setMusicVolumeFromMouse(sf::Vector2i pixel)
{
    const sf::FloatRect track = scaledBaseRect(kMusicTrackBounds);
    const float ratio = (pixel.x - track.position.x) / track.size.x;
    cfg_.musicVolume = std::clamp(static_cast<int>(std::lround(ratio * 100.f)), 0, 100);
    applyAudioVolumes();
}

void UIManager::applyAudioVolumes()
{
    // Settings cua Player la phan tram (0..100). AudioManager nhan them
    // Base Volume theo category de tao Final_Volume cho tung asset.
    audio_.setUserVolumes(static_cast<float>(cfg_.musicVolume),
                          static_cast<float>(cfg_.volume));
}

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

int UIManager::menuButtonAt(sf::Vector2i pixel) const
{
    const sf::Vector2f point(static_cast<float>(pixel.x), static_cast<float>(pixel.y));
    for (std::size_t i = 0; i < kMainMenuButtonBounds.size(); ++i)
        if (scaledMenuBounds(i).contains(point)) return static_cast<int>(i);
    return -1;
}

void UIManager::activateMainMenuButton(std::size_t index)
{
    focusIdx_ = static_cast<int>(index);
    switch (index)
    {
    case 0: setState(UIState::ModeSelect); break;                         // Play
    case 1: loadTabModeIdx_ = 0; setState(UIState::LoadGame); break;      // Load
    case 2: setState(UIState::Graphic); break;                             // Graphic
    case 3: setState(UIState::Setting); break;                             // Setting
    case 4: rankTabModeIdx_ = 0; rankScrollOffset_ = 0; setState(UIState::Ranking); break;
    case 5: setState(UIState::Help); break;
    case 6: modal_ = Modal::ConfirmExit; break;
    default: break;
    }
}

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

void UIManager::activateFocused()
{
    if (btns_.empty()) return;
    if (focusIdx_ < 0 || focusIdx_ >= (int)btns_.size()) return;
    if (!btns_[focusIdx_].isEnabled()) return;
    btns_[focusIdx_].consumeEnter();
    // Each button was created with a capturing lambda; calling its callback
    // is done by binding a std::function during rebuildButtons().
    if (focusIdx_ < (int)btnActions_.size() && btnActions_[focusIdx_])
        btnActions_[focusIdx_]();
}

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
        add(y(2), { BTN_W, BTN_H }, "Ranking", Button::Style::Primary, [this] { rankTabModeIdx_ = 0; rankScrollOffset_ = 0; setState(UIState::Ranking); });
        add(y(3), { BTN_W, BTN_H }, "Setting", Button::Style::Primary, [this] { setState(UIState::Setting); });
        add(y(4), { BTN_W, BTN_H }, "Graphic", Button::Style::Primary, [this] { setState(UIState::Graphic); });
        add(y(5), { BTN_W, BTN_H }, "Help", Button::Style::Primary, [this] { setState(UIState::Help); });
        add(y(6), { BTN_W, BTN_H }, "Exit", Button::Style::Danger, [this] { modal_ = Modal::ConfirmExit; });
        break;
    }

    case UIState::ModeSelect:
    {
        auto y = vstack(260, 2);
        add(y(0), { BTN_W, BTN_H }, "Classic Mode", Button::Style::Primary, [this] {
            ctx_.mode = StateContext::Mode::Classic;
            ctx_.selectedCharacterID = cfg_.cosmetic.characterId;
            nameBuffer_.clear();
            setState(UIState::NameInput);
            });
        add(y(1), { BTN_W, BTN_H }, "Endless Mode", Button::Style::Primary, [this] {
            ctx_.mode = StateContext::Mode::Endless;
            ctx_.selectedCharacterID = cfg_.cosmetic.characterId;
            nameBuffer_.clear();
            setState(UIState::NameInput);
            });
        break;
    }

    case UIState::LevelSelect:
    {
        // 2 rows x 5 cols.
        for (int i = 0; i < CLASSIC_LEVELS; ++i)
        {
            const int row = i / 5;
            const int col = i % 5;
            const float x = 240.f + col * 160.f;
            const float y = 240.f + row * 160.f;
            const int  level = i + 1;
            const bool unlocked = level <= prog_.highestUnlockedLevel();
            std::string label = std::to_string(level);
            if (unlocked) label += "  [done]";
            add({ x, y }, { 120, 120 }, label, Button::Style::Primary, [this, level] {
                if (level <= prog_.highestUnlockedLevel())
                {
                    ctx_.level = level;
                    setState(UIState::ClassicPlay);
                }
                }, unlocked);
        }
        break;
    }

    case UIState::Setting:
    {
        auto y = vstack(400, 1);
        add(y(0), { BTN_W, BTN_H }, "Back", Button::Style::Subtle, [this] { handleBack(); });
        break;
    }

    case UIState::Graphic:
    {
        auto y = vstack(240, 3);
        add(y(0), { BTN_W, BTN_H }, "Character  ( < / > )", Button::Style::Subtle, [this] {
            cfg_.cosmetic.characterId = CharacterRenderer::normalizeID(cfg_.cosmetic.characterId + 1);
            ctx_.selectedCharacterID = cfg_.cosmetic.characterId;
            sets_.save(cfg_);
            });
        add(y(1), { BTN_W, BTN_H }, "Background  ( < / > )", Button::Style::Subtle, [this] {
            cfg_.cosmetic.backgroundId = std::min(7, cfg_.cosmetic.backgroundId + 1);
            sets_.save(cfg_);
            });
        add(y(2), { BTN_W, BTN_H }, "Back", Button::Style::Subtle, [this] { handleBack(); });
        break;
    }

    case UIState::LoadGame:
    {
        // First two buttons: tab selectors.
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

            // ĐỔI 0 THÀNH i ĐỂ HIỂN THỊ ĐÚNG DATA CỦA 3 SLOT
            if (!slots[i].name.empty())
            {
                label = "Slot " + std::to_string(i + 1) + ": " + slots[i].name
                    + "  L" + std::to_string(slots[i].level)
                    + "  " + std::to_string(slots[i].elapsedSec) + "s";
            }

            // ĐỔI 0 THÀNH i ĐỂ MỞ KHÓA CLICK CHO CẢ 3 SLOT
            const bool filled = !slots[i].name.empty();

            add({ x, y }, { 240, 120 }, label, Button::Style::Primary, [this, m, i] {
                const auto s2 = saves_.slots(m);
                // ĐỔI 0 THÀNH i
                if (!s2[i].name.empty())
                {
                    ctx_.pendingName = s2[i].name;
                    ctx_.level = s2[i].level;
                    ctx_.classicLevel = s2[i].level;
                    ctx_.classicSec = s2[i].elapsedSec;
                    ctx_.mode = (m == GameMode::Endless)
                        ? StateContext::Mode::Endless
                        : StateContext::Mode::Classic;

                    // Hàm này gọi resetGameplay(), đưa nhân vật về vạch xuất phát
                    setState(m == GameMode::Endless
                        ? UIState::EndlessPlay
                        : UIState::ClassicPlay);

                    // NGAY SAU KHI SETSTATE, GHI ĐÈ LẠI TỌA ĐỘ VÀ THỜI GIAN ĐÃ LƯU
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

    case UIState::Ranking:
    {
        add({ 200, 150 }, { 200, 48 }, rankTabModeIdx_ == 0 ? "[X] Classic" : "[ ] Classic",
            Button::Style::Subtle, [this] { rankTabModeIdx_ = 0; rankScrollOffset_ = 0; rebuildButtons(); });
        add({ 420, 150 }, { 200, 48 }, rankTabModeIdx_ == 1 ? "[X] Endless" : "[ ] Endless",
            Button::Style::Subtle, [this] { rankTabModeIdx_ = 1; rankScrollOffset_ = 0; rebuildButtons(); });
        break;
    }

    case UIState::Help:
    {
        add({ (UI_W - BTN_W) * 0.5f, 600 }, { BTN_W, BTN_H }, "Back",
            Button::Style::Subtle, [this] { handleBack(); });
        break;
    }

    default:
        break;
    }

    for (int i = 0; i < (int)btns_.size(); ++i) btns_[i].setFocused(i == focusIdx_);
}

// ---------------------------------------------------------------------
//  Text input validation
// ---------------------------------------------------------------------
bool UIManager::isValidNameChar(std::uint32_t c)
{
    if (c == ' ') return true;
    if (c >= '0' && c <= '9') return true;
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= 'a' && c <= 'z') return true;
    if (c == '_' || c == '-' || c == '.') return true;
    return false;
}

bool UIManager::isValidName(const std::string& s)
{
    if (s.empty() || (int)s.size() > NAME_MAX) return false;
    for (char c : s) if (!isValidNameChar((std::uint32_t)c)) return false;
    return true;
}

// ---------------------------------------------------------------------
//  Drawing helpers
// ---------------------------------------------------------------------
void UIManager::drawBackground()
{
    if (assetsLoaded_)
    {
        const sf::Texture* texture = &bgTex_;
        if (state_ == UIState::Setting) texture = &settingBgTex_;
        else if (state_ == UIState::Graphic) texture = &graphicBgTex_;

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

void UIManager::drawMainMenuHoverGlow()
{
    const int hovered = menuButtonAt(sf::Mouse::getPosition(win_));
    if (hovered < 0) return; // Normal state is fully transparent.

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

void UIManager::drawDebugAudioMixer()
{
    if (!fontLoaded_) return;

    // Toa do theo pixel cua cua so de overlay luon de doc o ca man hinh
    // gameplay (default view) lan cac man UI (uiView_).
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
    if (state_ == UIState::Graphic) {
        box(kCharacterPanelBounds); box(kCharacterPrevBounds); box(kCharacterNextBounds);
        box(kThemePanelBounds); box(kThemePrevBounds); box(kThemeNextBounds);
    } else if (state_ == UIState::Setting) {
        box(kSfxTrackBounds); box(kSfxDecBounds); box(kSfxIncBounds);
        box(kMusicTrackBounds); box(kMusicDecBounds); box(kMusicIncBounds); box(kSettingOkBounds);
    }
}

void UIManager::drawBackIcon()
{
    if (state_ == UIState::Boot || state_ == UIState::MainMenu || state_ == UIState::Pause ||
        state_ == UIState::ClassicPlay || state_ == UIState::EndlessPlay)
        return;
    Button b(font_, { BACK_X, BACK_Y }, { BACK_SIZE, BACK_SIZE }, "<", Button::Style::IconOnly);
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
    {
        sf::Vector2f mp = (sf::Vector2f)sf::Mouse::getPosition(win_);
        if (b.contains(mp)) handleBack();
    }
    if (e_isMouseOverBack) b.setFocused(true);
    e_isMouseOverBack = b.contains((sf::Vector2f)sf::Mouse::getPosition(win_));
    b.draw(win_, font_);
}

void UIManager::drawModalOverlay()
{
    if (modal_ != Modal::ConfirmExit) return;
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

// ---------------------------------------------------------------------
//  Top-level render dispatch
// ---------------------------------------------------------------------
void UIManager::render()
{
    const bool gameplay = state_ == UIState::ClassicPlay || state_ == UIState::EndlessPlay;
    if (gameplay)
    {
        win_.setView(win_.getDefaultView());
        win_.clear(sf::Color(21, 25, 31));
    }
    else
    {
        // Backgrounds use physical window pixels so they fill any resolution.
        win_.setView(win_.getDefaultView());
        drawBackground();
        win_.setView(uiView_);
    }

    switch (state_)
    {
    case UIState::Boot:        renderBoot();        break;
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

    if (!gameplay) {
        drawBackIcon();
        drawModalOverlay();
    }

    // The transparent menu hitboxes need only a hover layer. Draw it after
    // the menu image, but before debug information.
    if (state_ == UIState::MainMenu)
    {
        win_.setView(win_.getDefaultView());
        drawMainMenuHoverGlow();
    }

    // Last draw calls in the frame: debug text cannot be hidden by a
    // background, modal, button or hover effect. It also works in gameplay.
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

// ---------------------------------------------------------------------
//  Per-screen render
// ---------------------------------------------------------------------
void UIManager::renderBoot()
{
    if (assetsLoaded_)
    {
        sf::Sprite logo(logoTex_);
        logo.setScale({ 0.5f, 0.5f });
        const auto sz = logoTex_.getSize();
        logo.setPosition({ (UI_W - sz.x * 0.5f) * 0.5f, 180.f });
        win_.draw(logo);
    }
    drawCenteredText("BRAKING BAD",  520, 56, colorFromHex(0xB41E1E), true);
    drawCenteredText("Team: 25C11_OOP", 600, 24, sf::Color::White);
    drawCenteredText("(c) 2026",        640, 18, sf::Color(180, 180, 180));
}

void UIManager::renderMainMenu()
{
    // Artwork and labels are part of the seasonal MainMenu background.
}

void UIManager::renderModeSel()
{
    drawCenteredText("SELECT MODE", 160, 44, sf::Color::White, true);
    for (auto& b : btns_) b.draw(win_, font_);
}

void UIManager::renderName()
{
    drawCenteredText("ENTER YOUR NAME", 180, 40, sf::Color::White, true);
    sf::RectangleShape box({ 600, 60 });
    box.setPosition({ (UI_W - 600) * 0.5f, 280 });
    box.setFillColor(sf::Color(20, 20, 30, 220));
    box.setOutlineThickness(-2.f);
    box.setOutlineColor(sf::Color(180, 200, 240));
    win_.draw(box);

    sf::Text t(font_, nameBuffer_ + "_", 28);
    t.setFillColor(sf::Color::White);
    t.setPosition({ (UI_W - 600) * 0.5f + 16, 292 });
    win_.draw(t);

    drawCenteredText("Enter to confirm  |  Backspace to edit  |  Esc to go back",
                     400, 18, sf::Color(180, 180, 180));
    drawCenteredText(std::string("Mode: ") + (ctx_.mode == StateContext::Mode::Classic ? "Classic" : "Endless"),
                     440, 18, sf::Color(200, 200, 200));
}

void UIManager::renderLvlSel()
{
    drawCenteredText("SELECT LEVEL", 160, 40, sf::Color::White, true);
    drawCenteredText("Completed levels show [done]", 200, 18, sf::Color(200, 200, 200));
    for (auto& b : btns_) b.draw(win_, font_);
}

void UIManager::renderSetting()
{
    win_.setView(win_.getDefaultView());
    auto slider = [&](const sf::FloatRect& base, int value, const std::string& name) {
        const sf::FloatRect b = scaledBaseRect(base);
        sf::RectangleShape track(b.size); track.setPosition(b.position); track.setFillColor(sf::Color(20,20,20,190)); win_.draw(track);
        sf::RectangleShape fill({b.size.x * value / 100.f, b.size.y}); fill.setPosition(b.position); fill.setFillColor(sf::Color(255,215,0,180)); win_.draw(fill);
        sf::CircleShape knob(13.f); knob.setOrigin({13.f,13.f}); knob.setPosition({b.position.x + b.size.x * value / 100.f,b.position.y + b.size.y*.5f}); knob.setFillColor(sf::Color::White); win_.draw(knob);
        sf::Text label(font_, name + ": " + std::to_string(value) + "%", 22); label.setFillColor(sf::Color::White); label.setPosition({b.position.x,b.position.y-32.f}); win_.draw(label);
    };
    slider(kSfxTrackBounds, cfg_.volume, "SFX");
    slider(kMusicTrackBounds, cfg_.musicVolume, "Music");
    win_.setView(uiView_);
}

void UIManager::renderGraphic()
{
    win_.setView(win_.getDefaultView());
    const sf::FloatRect left = scaledBaseRect(kCharacterPanelBounds), right = scaledBaseRect(kThemePanelBounds);
    sf::Text a(font_, "CHARACTER: " + CharacterRenderer::name(ctx_.selectedCharacterID), 28); a.setFillColor(sf::Color::White); a.setPosition({left.position.x+35.f,left.position.y+35.f}); win_.draw(a);
    sf::Text b(font_, "THEME: " + kThemeNames[currentThemeIndex_], 28); b.setFillColor(sf::Color::White); b.setPosition({right.position.x+35.f,right.position.y+35.f}); win_.draw(b);
    auto arrow = [&](const sf::FloatRect& base, const char* text) { const sf::FloatRect r=scaledBaseRect(base); sf::Text t(font_, text, 44); t.setFillColor(sf::Color::White); t.setPosition({r.position.x+16.f,r.position.y}); win_.draw(t); };
    arrow(kCharacterPrevBounds,"<"); arrow(kCharacterNextBounds,">"); arrow(kThemePrevBounds,"<"); arrow(kThemeNextBounds,">");
    win_.setView(uiView_);
}

void UIManager::renderLoad()
{
    drawCenteredText("LOAD GAME", 100, 40, sf::Color::White, true);
    drawCenteredText("Tab to switch  |  Del to remove  |  Click to load", 200, 18, sf::Color(200, 200, 200));
    for (auto& b : btns_) b.draw(win_, font_);
}

void UIManager::renderRanking()
{
    const GameMode m = (rankTabModeIdx_ == 0) ? GameMode::Classic : GameMode::Endless;
    const std::string title = (m == GameMode::Classic) ? "RANKING  -  CLASSIC" : "RANKING  -  ENDLESS";
    drawCenteredText(title, 100, 36, sf::Color::White, true);

    for (auto& b : btns_) b.draw(win_, font_);

    const auto rows = ranks_.all(m);
    const int total    = (int)rows.size();
    const int maxOff   = std::max(0, total - RankingStore::kMaxVisible);
    rankScrollOffset_  = std::clamp(rankScrollOffset_, 0, maxOff);

    const int end = std::min(total, rankScrollOffset_ + RankingStore::kMaxVisible);
    const float baseY = 240.f;
    const float rowH  = 36.f;

    std::string headerStr = (m == GameMode::Classic)
        ? "  #   Name                 Lvl     Time"
        : "  #   Name                 Time";
    sf::Text header(font_, headerStr, 18);
    header.setFillColor(sf::Color(200, 200, 200));
    header.setPosition({ 220, baseY - 30 });
    win_.draw(header);

    for (int i = rankScrollOffset_; i < end; ++i)
    {
        const auto& r = rows[i];
        std::string line = "  " + std::to_string(i + 1) + ".  " + r.name;

        // Căn lề cột Name (khoảng 26 ký tự)
        while ((int)line.size() < 26) line += ' ';

        if (m == GameMode::Classic) {
            std::string lvlStr = std::to_string(r.level);
            // Căn lề cột Lvl (khoảng 8 ký tự)
            while ((int)lvlStr.size() < 8) lvlStr += ' ';
            line += lvlStr + std::to_string(r.elapsedSec) + "s";
        }
        else {
            // Chế độ Endless chỉ in thời gian sống
            line += std::to_string(r.elapsedSec) + "s";
        }

        // (Đã xóa bỏ hoàn toàn phần in r.savedAtUnix ở đây)

        sf::Text row(font_, line, 20);
        row.setFillColor((i == rankScrollOffset_) ? colorFromHex(0xFFD24A) : sf::Color::White);
        row.setPosition({ 220, baseY + (i - rankScrollOffset_) * rowH });
        win_.draw(row);
    }

    // Scrollbar.
    if (total > RankingStore::kMaxVisible)
    {
        const float barX = UI_W - 60;
        const float barY = 240;
        const float barH = rowH * RankingStore::kMaxVisible;
        sf::RectangleShape track({ 6, barH });
        track.setPosition({ barX, barY });
        track.setFillColor(sf::Color(60, 60, 60));
        win_.draw(track);
        const float thumbH = std::max(20.f, barH * (RankingStore::kMaxVisible / (float)total));
        const float thumbY = barY + (barH - thumbH) * (rankScrollOffset_ / (float)maxOff);
        sf::RectangleShape thumb({ 10, thumbH });
        thumb.setPosition({ barX - 2, thumbY });
        thumb.setFillColor(sf::Color(180, 180, 220));
        win_.draw(thumb);
    }

    drawCenteredText("Wheel / Up-Down / PageUp-Down to scroll", 660, 18, sf::Color(180, 180, 180));
}

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

void UIManager::drawMapBlock(const MapBlock& block, float cameraY)
{
    const float screenY = block.startY - cameraY;
    if (screenY >= Grid::MAP_HEIGHT || screenY + block.height() <= 0.f)
        return;

    // --- OPTIMIZATION: DECLARE SHAPES ONCE ---
    // Instantiating these here prevents SFML from recalculating geometry 
    // vertices inside the loops!
    sf::RectangleShape lane({ Grid::MAP_WIDTH, Grid::CELL_SIZE });

    sf::CircleShape manhole(Grid::CELL_SIZE * 0.35f);
    manhole.setOutlineThickness(4.f);
    manhole.setOrigin({ manhole.getRadius(), manhole.getRadius() });

    sf::RectangleShape strip;
    sf::RectangleShape line;
    // -----------------------------------------

    for (int row = 0; row < LANES_PER_BLOCK; ++row) {
        lane.setPosition({ 0.f, screenY + row * Grid::CELL_SIZE });
        lane.setFillColor(laneColor(block.lanes[row], block.biome));
        win_.draw(lane);

        // Vẽ nắp cống
        if (block.manholeCols[row] != -1) {
            int col = block.manholeCols[row];
            if (Grid::isPlayableColumn(col)) {
                manhole.setFillColor(sf::Color(60, 60, 60));
                manhole.setOutlineColor(sf::Color(30, 30, 30));

                float cx = Grid::columnCenter(col);
                float cy = screenY + (row + 0.5f) * Grid::CELL_SIZE;
                manhole.setPosition({ cx, cy });

                win_.draw(manhole);
            }
        }
    }

    const float sideWidth = Grid::PLAYABLE_FIRST_COLUMN * Grid::CELL_SIZE;

    strip.setSize({ sideWidth, block.height() });
    strip.setFillColor(sf::Color(0, 0, 0, 70));

    strip.setPosition({ 0.f, screenY });
    win_.draw(strip);

    strip.setPosition({ Grid::MAP_WIDTH - sideWidth, screenY });
    win_.draw(strip);

    line.setFillColor(sf::Color(255, 255, 255, 95));
    line.setSize({ 2.f, block.height() });
    for (int col = 0; col <= Grid::COLUMNS; ++col) {
        line.setPosition({ Grid::GRID_LEFT + col * Grid::CELL_SIZE, screenY });
        win_.draw(line);
    }

    line.setSize({ Grid::GRID_WIDTH, 2.f });
    for (int row = 0; row <= Grid::ROWS_PER_BLOCK; ++row) {
        line.setPosition({ Grid::GRID_LEFT, screenY + row * Grid::CELL_SIZE });
        win_.draw(line);
    }
}

void UIManager::drawPlayer()
{
    player_.draw(win_, cameraY_);
}

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

    // Draw the test obstacles with camera offset
    for (auto obs : Obstacles) {
        obs->draw(win_, cameraY_);
    }

    drawPlayer();

    // --- DEBUG HITBOX DRAWING ---
    if (debugUi_) {
        // 1. Draw NEON MAGENTA Hitboxes for all Obstacles
        for (auto obs : Obstacles) {
            sf::FloatRect bounds = obs->getBounds();
            sf::RectangleShape box(bounds.size);
            box.setPosition({ bounds.position.x, bounds.position.y - cameraY_ });
            box.setFillColor(sf::Color::Transparent);
            box.setOutlineColor(sf::Color::Magenta);
            box.setOutlineThickness(-4.0f); // NEGATIVE draws outwards!
            win_.draw(box);
        }

        // 2. Draw NEON CYAN Hitbox for the Player
        sf::FloatRect pBounds = player_.getBounds();
        sf::RectangleShape pBox(pBounds.size);
        pBox.setPosition({ pBounds.position.x, pBounds.position.y - cameraY_ });
        pBox.setFillColor(sf::Color::Transparent);
        pBox.setOutlineColor(sf::Color::Cyan);
        pBox.setOutlineThickness(-4.0f); // NEGATIVE draws outwards!
        win_.draw(pBox);
    }
    // ----------------------------

    // Dedicated, 600px gameplay sidebar.
    sf::RectangleShape sidebar({ Grid::SIDEBAR_WIDTH, Grid::MAP_HEIGHT });
    sidebar.setPosition({ Grid::MAP_WIDTH, 0.f });
    sidebar.setFillColor(sf::Color(26, 31, 40));
    win_.draw(sidebar);

    sf::RectangleShape divider({ 4.f, Grid::MAP_HEIGHT });
    divider.setPosition({ Grid::MAP_WIDTH - 2.f, 0.f });
    divider.setFillColor(colorFromHex(0xE8B44F));
    win_.draw(divider);

    const int wholeSeconds = static_cast<int>(elapsedPlaySec_);
    const int minutes = wholeSeconds / 60;
    const int seconds = wholeSeconds % 60;
    const std::string timer = "TIME  " + std::to_string(minutes / 10) +
        std::to_string(minutes % 10) + ":" +
        std::to_string(seconds / 10) + std::to_string(seconds % 10);
    sf::Text timerText(font_, timer, 48);
    timerText.setFillColor(sf::Color::White);
    timerText.setPosition({ Grid::MAP_WIDTH + 72.f, 110.f });
    win_.draw(timerText);

    sf::Text pauseText(font_, "ESC to Pause", 30);
    pauseText.setFillColor(sf::Color(210, 215, 225));
    pauseText.setPosition({ Grid::MAP_WIDTH + 72.f, 205.f });
    win_.draw(pauseText);

    sf::Text movementText(font_, gameplayStarted_ ? "Arrow keys: move" : "Press an arrow key to start", 24);
    movementText.setFillColor(sf::Color(170, 180, 195));
    movementText.setPosition({ Grid::MAP_WIDTH + 72.f, 265.f });
    win_.draw(movementText);
    win_.draw(trafficLightSprite_);
}


void UIManager::renderGameOver()
{
    drawCenteredText(classicWon_ ? "LEVEL COMPLETE" : "GAME OVER", 220, 56,
                     classicWon_ ? colorFromHex(0x4CAF50) : colorFromHex(0xB41E1E), true);
    drawCenteredText("Player: " + ctx_.pendingName, 320, 22, sf::Color::White);
    if (ctx_.mode == StateContext::Mode::Classic)
    {
        drawCenteredText("Reached level: " + std::to_string(ctx_.classicLevel), 360, 22, sf::Color::White);
        drawCenteredText("Time: " + std::to_string(ctx_.classicSec) + "s", 390, 22, sf::Color::White);
    }
    else
    {
        drawCenteredText("Time:  " + std::to_string(ctx_.endlessSec) + "s", 390, 22, sf::Color::White);
    }
    drawCenteredText(classicWon_ ? "Enter to return to level select"
                                 : "Enter to submit score and return to menu",
                     460, 20, sf::Color(200, 200, 200));
}

void UIManager::renderPause()
{
    drawCenteredText("PAUSED", 280, 56, sf::Color::White, true);
    drawCenteredText("Enter to resume   |   Esc to main menu", 360, 22, sf::Color(200, 200, 200));
}
