#include "UIManager.h"
#include "CAnimal.h"
#include "CVehicle.h"
#include "Grid.h"
#include <SFML/Graphics.hpp>
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
    constexpr float         PLAYER_STEP = Grid::CELL_SIZE;
    constexpr float         ENDLESS_SCROLL_SPEED = 180.f; // px/s, world Y decreases
    constexpr float         ENDLESS_CATCHUP_SPEED = 1500.f;
    constexpr float         CLASSIC_FOLLOW_SPEED = 960.f;  // px/s, catches up to player
    constexpr float         PLAYER_SCREEN_ANCHOR = Grid::MAP_HEIGHT * 0.65f;
    constexpr float         PLAYER_TOP_SAFE_LINE = 180.f;
    constexpr float         PLAYER_RADIUS = 42.f;
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
            return biome == BiomeType::Desert ? colorFromHex(0xC9A66B)
                 : biome == BiomeType::Swamp  ? colorFromHex(0x4E8063)
                                              : colorFromHex(0x57845A);
        case LaneType::Vehicle: return colorFromHex(0x4A4E57); // asphalt
        case LaneType::Animal:  return colorFromHex(0x8A704E); // dirt/field
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
    sf::FloatRect({ 1416.f, 540.f }, { 314.f, 105.f }),
    sf::FloatRect({ 1416.f, 656.f }, { 314.f, 105.f }),
    sf::FloatRect({ 1416.f, 781.f }, { 314.f, 105.f }),
    // Bottom-right round icons: Leaderboard, Help, Exit.
    sf::FloatRect({ 1555.f, 980.f }, { 94.f, 94.f }),
    sf::FloatRect({ 1684.f, 980.f }, { 94.f, 94.f }),
    sf::FloatRect({ 1807.f, 980.f }, { 94.f, 94.f })
};

UIManager::UIManager(sf::RenderWindow& window)
    : win_(window),
      uiView_({UI_W * 0.5f, UI_H * 0.5f}, {UI_W, UI_H}),
      debugText_(font_, "", 18)
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

    const bool lg  = logoTex_.loadFromFile("Graphic/1x/DataList.png");
    (void)        iconsTex_.loadFromFile("Graphic/1x/DataList.png");
    (void)lg; // The seasonal backgrounds are the only required UI images.
    setTheme("autumn");

    cfg_ = sets_.load();
    cfg_.cosmetic.characterId = std::clamp(cfg_.cosmetic.characterId, 1,
                                           CharacterRenderer::kCharacterCount);
    ctx_.selectedCharacterID = cfg_.cosmetic.characterId;
    saves_.loadAll();
    ranks_.loadAll();
    prog_.load();

    setState(UIState::Boot);
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
        if (k->code == sf::Keyboard::Key::Enter)  { activateFocused(); return; }
        if (k->code == sf::Keyboard::Key::Up)     { moveFocus(-1); return; }
        if (k->code == sf::Keyboard::Key::Down)   { moveFocus(+1); return; }
        if (k->code == sf::Keyboard::Key::Left)   { cfg_.volume = std::max(0,   cfg_.volume - 5); sets_.save(cfg_); return; }
        if (k->code == sf::Keyboard::Key::Right)  { cfg_.volume = std::min(100, cfg_.volume + 5); sets_.save(cfg_); return; }
    }
    if (const auto* mm = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mm->button == sf::Mouse::Button::Left)
        {
            sf::Vector2f mp(static_cast<float>(mm->position.x),
                            static_cast<float>(mm->position.y));
            for (int i = 0; i < (int)btns_.size(); ++i)
            {
                if (btns_[i].consumeClick(mp)) { focusIdx_ = i; activateFocused(); return; }
            }
            // Click on the slider track to jump volume.
            sf::FloatRect track({ 500, 200 }, { 600, 40 });
            const sf::Vector2f uiMp = toUiCoords(mm->position);
            if (track.contains(uiMp))
            {
                float t = (uiMp.x - track.position.x) / track.size.x;
                cfg_.volume = std::clamp(static_cast<int>(t * 100.f), 0, 100);
                sets_.save(cfg_);
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

void UIManager::handleGraphic(const sf::Event& e)
{
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Escape) { handleBack(); return; }
        if (k->code == sf::Keyboard::Key::Enter)  { activateFocused(); return; }
        if (k->code == sf::Keyboard::Key::Up)     { moveFocus(-1); return; }
        if (k->code == sf::Keyboard::Key::Down)   { moveFocus(+1); return; }
        if (k->code == sf::Keyboard::Key::Left)
        {
            if (focusIdx_ == 0) {
                cfg_.cosmetic.characterId = CharacterRenderer::normalizeID(cfg_.cosmetic.characterId - 1);
                ctx_.selectedCharacterID = cfg_.cosmetic.characterId;
                sets_.save(cfg_);
            }
            if (focusIdx_ == 1) { cfg_.cosmetic.backgroundId = std::max(0, cfg_.cosmetic.backgroundId - 1); sets_.save(cfg_); }
            return;
        }
        if (k->code == sf::Keyboard::Key::Right)
        {
            if (focusIdx_ == 0) {
                cfg_.cosmetic.characterId = CharacterRenderer::normalizeID(cfg_.cosmetic.characterId + 1);
                ctx_.selectedCharacterID = cfg_.cosmetic.characterId;
                sets_.save(cfg_);
            }
            if (focusIdx_ == 1) { cfg_.cosmetic.backgroundId = std::min(7, cfg_.cosmetic.backgroundId + 1); sets_.save(cfg_); }
            return;
        }
    }
    if (const auto* mm = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mm->button == sf::Mouse::Button::Left)
        {
            sf::Vector2f mp(static_cast<float>(mm->position.x),
                            static_cast<float>(mm->position.y));
            for (int i = 0; i < (int)btns_.size(); ++i)
            {
                if (btns_[i].consumeClick(mp)) { focusIdx_ = i; activateFocused(); return; }
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

        sf::Vector2f next = playerWorldPos_;
        const bool isMove = k->code == sf::Keyboard::Key::Left ||
                            k->code == sf::Keyboard::Key::Right ||
                            k->code == sf::Keyboard::Key::Up ||
                            k->code == sf::Keyboard::Key::Down;
        if (!isMove) return;

        // Once the player is fully below the viewport, ignore every movement
        // key. This prevents clamping an off-screen Y back onto a visible row.
        if (state_ == UIState::EndlessPlay && isEndlessPlayerOffscreen())
        {
            finishEndlessRun();
            return;
        }

        // The first directional input starts both clock and camera. World
        // position itself is only changed below, never by camera updates.
        gameplayStarted_ = true;
        if (k->code == sf::Keyboard::Key::Left)  next.x -= PLAYER_STEP;
        if (k->code == sf::Keyboard::Key::Right) next.x += PLAYER_STEP;
        if (k->code == sf::Keyboard::Key::Up)    next.y -= PLAYER_STEP;
        if (k->code == sf::Keyboard::Key::Down)  next.y += PLAYER_STEP;

        // Columns 0-1 and 9-10 remain obstacle-only lanes.
        next.x = std::clamp(next.x, Grid::playableLeftCenter(), Grid::playableRightCenter());

        const float topLimit = state_ == UIState::ClassicPlay
            ? classicMap_.topLimit() + Grid::CELL_SIZE * 0.5f
            : -std::numeric_limits<float>::infinity();
        const float mapBottom = state_ == UIState::ClassicPlay
            ? classicMap_.bottomLimit() - Grid::CELL_SIZE * 0.5f
            : -Grid::CELL_SIZE * 0.5f;
        // maxWalkablePlayerY() only returns a row that is 100% visible. It
        // therefore prevents stepping into a partially clipped bottom tile.
        const float minY = topLimit;
        const float maxY = std::min(mapBottom, maxWalkablePlayerY());
        const bool verticalMove = k->code == sf::Keyboard::Key::Up ||
                                  k->code == sf::Keyboard::Key::Down;
        // Reject invalid vertical movement instead of snapping the player's
        // current row to maxY; this removes the off-screen teleport.
        if (verticalMove && next.y >= minY && next.y <= maxY)
            playerWorldPos_.y = next.y;
        playerWorldPos_.x = next.x;

        // The centre of the final block's top row is the Classic finish tile.
        if (state_ == UIState::ClassicPlay &&
            playerWorldPos_.y <= classicMap_.topLimit() + Grid::CELL_SIZE * 0.5f)
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
    if (const auto* k = e.getIf<sf::Event::KeyPressed>())
    {
        if (k->code == sf::Keyboard::Key::Enter || k->code == sf::Keyboard::Key::R)
        {
            if (classicWon_)
            {
                classicWon_ = false;
                setState(UIState::LevelSelect);
                return;
            }

            // Submit to ranking before leaving.
            RunRecord r;
            r.name = ctx_.pendingName;
            if (ctx_.mode == StateContext::Mode::Classic)
            {
                r.mode       = GameMode::Classic;
                r.level      = ctx_.classicLevel;
                r.elapsedSec = ctx_.classicSec;
                r.score      = 0;
            }
            else
            {
                r.mode       = GameMode::Endless;
                r.level      = 0;
                r.elapsedSec = ctx_.endlessSec;
                r.score      = ctx_.endlessScore;
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
        if (k->code == sf::Keyboard::Key::Escape) handleBack();
        if (k->code == sf::Keyboard::Key::Enter)
        {
            // Resume: go back to the gameplay state we came from.
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
        elapsedPlaySec_ += dt;
        updateCamera(dt);
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

            // 1. Move all obstacles first
            for (auto obs : Obstacles) {
                obs->move(dt);
            }

            // 2. Clean up off-screen obstacles safely using an Iterator
            for (auto it = Obstacles.begin(); it != Obstacles.end(); ) {
                if ((*it)->isOffScreen()) {
                    delete* it;
                    it = Obstacles.erase(it);
                }
                else {
                    ++it;
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

                            // We only spawn in Vehicle and Animal lanes
                            if (type == LaneType::Safe) continue;

                            // 1. FIX THE DIRECTION: Alternate direction based on the row and block ID!
                            direction dir = ((block.blockID + row) % 2 == 0) ? RIGHT : LEFT;

                            // 50% chance to attempt a spawn in this lane
                            if (rand() % 100 < 50) {

                                float rowY = block.startY + (row * Grid::CELL_SIZE);
                                float spawnX = (dir == RIGHT) ? Grid::GRID_LEFT - 150.f : Grid::GRID_RIGHT + 150.f;

                                // 2. FIX THE OVERLAPPING: Check if the spawn point is currently blocked
                                bool isBlocked = false;
                                for (auto obs : Obstacles) {
                                    // If they are in the exact same lane...
                                    if (std::abs(obs->getY() - rowY) < 1.0f) {
                                        // And they are too close to the spawn point (within 250 pixels)
                                        if (std::abs(obs->getX() - spawnX) < 250.f) {
                                            isBlocked = true;
                                            break;
                                        }
                                    }
                                }

                                // 3. Spawn only if the road is clear!
                                if (!isBlocked) {
                                    if (type == LaneType::Vehicle) {
                                        Obstacles.push_back(new CVehicle(spawnX, rowY, dir));
                                    }
                                    else if (type == LaneType::Animal) {
                                        Obstacles.push_back(new CAnimal(spawnX, rowY, dir));
                                    }
                                }
                            }
                        }
                    }
                    };

                // Run the spawner on the correct map data
                if (state_ == UIState::EndlessPlay) {
                    spawnInBlocks(endlessMap_.getBlocks());
                }
                else {
                    spawnInBlocks(classicMap_.getBlocks());
                }
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
    playerWorldPos_ = { Grid::columnCenter(Grid::COLUMNS / 2), -Grid::CELL_SIZE * 0.5f };
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
    return playerWorldPos_.y - cameraY_ - PLAYER_RADIUS > Grid::MAP_HEIGHT;
}

void UIManager::finishEndlessRun()
{
    if (state_ != UIState::EndlessPlay) return;
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
        // catch-up, not a change to playerWorldPos_.
        const float playerScreenY = playerWorldPos_.y - cameraY_;
        if (playerScreenY < ENDLESS_CATCHUP_DISTANCE)
        {
            const float targetY = playerWorldPos_.y - PLAYER_SCREEN_ANCHOR;
            if (targetY < cameraY_)
                cameraY_ = std::max(targetY, cameraY_ - ENDLESS_CATCHUP_SPEED * dt);

            const float safeCameraY = playerWorldPos_.y - PLAYER_TOP_SAFE_LINE;
            if (cameraY_ > safeCameraY)
                cameraY_ = safeCameraY;
        }
        return;
    }

    // Classic follows the advancing player only. At the top block, its start
    // becomes the camera's immutable stop: that final 1080px block is fully
    // visible and remains so for the rest of the level.
    const float stopY = classicMap_.topLimit();
    const float targetY = std::max(stopY, playerWorldPos_.y - PLAYER_SCREEN_ANCHOR);
    if (targetY < cameraY_)
        cameraY_ = std::max(targetY, cameraY_ - CLASSIC_FOLLOW_SPEED * dt);

    // A rapid sequence of player moves cannot let the player leave through
    // the top while the smoothed camera is catching up.
    const float safeCameraY = std::max(stopY, playerWorldPos_.y - PLAYER_TOP_SAFE_LINE);
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
    const sf::Vector2u size = win_.getSize();
    const sf::FloatRect base = kMainMenuButtonBounds.at(index);
    const float sx = size.x / static_cast<float>(WINDOW_W);
    const float sy = size.y / static_cast<float>(WINDOW_H);
    return { { base.position.x * sx, base.position.y * sy },
             { base.size.x * sx, base.size.y * sy } };
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
        // Main-menu buttons are painted into MainMenu.png.  Their hit areas
        // live in kMainMenuButtonBounds and therefore deliberately have no
        // Button objects (and no visible fill or outline) here.
        break;
    }

    case UIState::ModeSelect:
    {
        auto y = vstack(260, 2);
        add(y(0), {BTN_W, BTN_H}, "Classic Mode", Button::Style::Primary, [this]{
            ctx_.mode = StateContext::Mode::Classic;
            ctx_.selectedCharacterID = cfg_.cosmetic.characterId;
            nameBuffer_.clear();
            setState(UIState::NameInput);
        });
        add(y(1), {BTN_W, BTN_H}, "Endless Mode", Button::Style::Primary, [this]{
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
            add({ x, y }, { 120, 120 }, label, Button::Style::Primary, [this, level]{
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
        add(y(0), {BTN_W, BTN_H}, "Back", Button::Style::Subtle, [this]{ handleBack(); });
        break;
    }

    case UIState::Graphic:
    {
        auto y = vstack(240, 3);
        add(y(0), {BTN_W, BTN_H}, "Character  ( < / > )", Button::Style::Subtle, [this]{
            cfg_.cosmetic.characterId = CharacterRenderer::normalizeID(cfg_.cosmetic.characterId + 1);
            ctx_.selectedCharacterID = cfg_.cosmetic.characterId;
            sets_.save(cfg_);
        });
        add(y(1), {BTN_W, BTN_H}, "Background  ( < / > )", Button::Style::Subtle, [this]{
            cfg_.cosmetic.backgroundId = std::min(7, cfg_.cosmetic.backgroundId + 1);
            sets_.save(cfg_);
        });
        add(y(2), {BTN_W, BTN_H}, "Back", Button::Style::Subtle, [this]{ handleBack(); });
        break;
    }

    case UIState::LoadGame:
    {
        // First two buttons: tab selectors.
        add({ 200, 150 }, { 200, 48 }, loadTabModeIdx_ == 0 ? "[X] Classic" : "[ ] Classic",
            Button::Style::Subtle, [this]{ loadTabModeIdx_ = 0; rebuildButtons(); });
        add({ 420, 150 }, { 200, 48 }, loadTabModeIdx_ == 1 ? "[X] Endless" : "[ ] Endless",
            Button::Style::Subtle, [this]{ loadTabModeIdx_ = 1; rebuildButtons(); });

        const GameMode m = (loadTabModeIdx_ == 0) ? GameMode::Classic : GameMode::Endless;
        const auto slots = saves_.slots(m);
        for (int i = 0; i < SaveStore::kMaxSlots; ++i)
        {
            const float x = 200.f + (i % 3) * 280.f;
            const float y = 260.f;
            std::string label = "Slot " + std::to_string(i + 1) + "  (empty)";
            if (i == 0 && slots[0].name.empty() == false)
            {
                label = "Slot " + std::to_string(i + 1) + ": " + slots[0].name
                      + "  L"  + std::to_string(slots[0].level)
                      + "  "   + std::to_string(slots[0].elapsedSec) + "s";
            }
            const bool filled = (i == 0 && !slots[0].name.empty());
            add({ x, y }, { 240, 120 }, label, Button::Style::Primary, [this, m, i]{
                const auto s2 = saves_.slots(m);
                if (i == 0 && !s2[0].name.empty())
                {
                    ctx_.pendingName  = s2[0].name;
                    ctx_.level        = s2[0].level;
                    ctx_.classicLevel = s2[0].level;
                    ctx_.classicSec   = s2[0].elapsedSec;
                    ctx_.mode = (m == GameMode::Endless)
                              ? StateContext::Mode::Endless
                              : StateContext::Mode::Classic;
                    setState(m == GameMode::Endless
                             ? UIState::EndlessPlay
                             : UIState::ClassicPlay);
                }
            }, filled);
        }
        break;
    }

    case UIState::Ranking:
    {
        add({ 200, 150 }, { 200, 48 }, rankTabModeIdx_ == 0 ? "[X] Classic" : "[ ] Classic",
            Button::Style::Subtle, [this]{ rankTabModeIdx_ = 0; rankScrollOffset_ = 0; rebuildButtons(); });
        add({ 420, 150 }, { 200, 48 }, rankTabModeIdx_ == 1 ? "[X] Endless" : "[ ] Endless",
            Button::Style::Subtle, [this]{ rankTabModeIdx_ = 1; rankScrollOffset_ = 0; rebuildButtons(); });
        break;
    }

    case UIState::Help:
    {
        add({ (UI_W - BTN_W)*0.5f, 600 }, { BTN_W, BTN_H }, "Back",
            Button::Style::Subtle, [this]{ handleBack(); });
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
        drawMouseDebugInfo();
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
    drawCenteredText("CROSSING DEAD",  520, 56, colorFromHex(0xB41E1E), true);
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
    drawCenteredText("SETTINGS", 140, 40, sf::Color::White, true);
    drawCenteredText("Master Volume", 200, 24, sf::Color::White);

    sf::FloatRect track({ 500, 250 }, { 600, 40 });
    sf::RectangleShape bar(track.size);
    bar.setPosition(track.position);
    bar.setFillColor(sf::Color(60, 60, 70));
    bar.setOutlineThickness(-2.f);
    bar.setOutlineColor(sf::Color(140, 140, 160));
    win_.draw(bar);

    float filled = track.size.x * (cfg_.volume / 100.f);
    sf::RectangleShape fill({ filled, track.size.y });
    fill.setPosition(track.position);
    fill.setFillColor(colorFromHex(0x4A90E2));
    win_.draw(fill);

    sf::Text v(font_, std::to_string(cfg_.volume) + "%", 22);
    v.setFillColor(sf::Color::White);
    v.setPosition({ track.position.x + track.size.x + 12, track.position.y + 6 });
    win_.draw(v);

    drawCenteredText("Click on the bar  |  Left/Right to step 5%  |  Esc back", 320, 18, sf::Color(180, 180, 180));

    for (auto& b : btns_) b.draw(win_, font_);
}

void UIManager::renderGraphic()
{
    drawCenteredText("GRAPHIC", 140, 40, sf::Color::White, true);
    CharacterRenderer::draw(win_, ctx_.selectedCharacterID, { UI_W * 0.5f, 205.f }, 54.f);
    drawCenteredText(CharacterRenderer::name(ctx_.selectedCharacterID),  290, 28, sf::Color::White, true);
    drawCenteredText("Background: #" + std::to_string(cfg_.cosmetic.backgroundId), 330, 24, sf::Color::White);
    drawCenteredText("Select Character or Background, then use < / >", 380, 18, sf::Color(180, 180, 180));
    for (auto& b : btns_) b.draw(win_, font_);
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

    sf::Text header(font_, "  #   Name                 Lvl   Time    Score   Date",
                    18);
    header.setFillColor(sf::Color(200, 200, 200));
    header.setPosition({ 220, baseY - 30 });
    win_.draw(header);

    for (int i = rankScrollOffset_; i < end; ++i)
    {
        const auto& r = rows[i];
        std::string line = "  " + std::to_string(i + 1) + ".  "
                         + r.name;
        // Pad name column to 20 chars.
        while ((int)line.size() < 26) line += ' ';
        line += "  " + std::to_string(r.level)
              + "  "  + std::to_string(r.elapsedSec) + "s"
              + "  "  + std::to_string(r.score)
              + "  "  + std::to_string((long long)r.savedAtUnix);
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

    // Each block owns a 9-row lane layout. Entity systems can later spawn
    // vehicles/animals only on their matching LaneType rows.
    sf::RectangleShape lane({Grid::MAP_WIDTH, Grid::CELL_SIZE});
    for (int row = 0; row < LANES_PER_BLOCK; ++row) {
        lane.setPosition({0.f, screenY + row * Grid::CELL_SIZE});
        lane.setFillColor(laneColor(block.lanes[row], block.biome));
        win_.draw(lane);
    }

    // All 11 columns are rendered (and available to obstacle systems). Only
    // the two outer columns on either side are darkened as non-walkable lanes.
    const float sideWidth = Grid::PLAYABLE_FIRST_COLUMN * Grid::CELL_SIZE;
    sf::RectangleShape leftStrip({sideWidth, block.height()});
    leftStrip.setPosition({0.f, screenY});
    leftStrip.setFillColor(sf::Color(0, 0, 0, 70));
    win_.draw(leftStrip);
    sf::RectangleShape rightStrip({sideWidth, block.height()});
    rightStrip.setPosition({Grid::MAP_WIDTH - sideWidth, screenY});
    rightStrip.setFillColor(sf::Color(0, 0, 0, 70));
    win_.draw(rightStrip);

    sf::RectangleShape line;
    line.setFillColor(sf::Color(255, 255, 255, 95));
    line.setSize({2.f, block.height()});
    for (int col = 0; col <= Grid::COLUMNS; ++col) {
        line.setPosition({Grid::GRID_LEFT + col * Grid::CELL_SIZE, screenY});
        win_.draw(line);
    }
    line.setSize({Grid::GRID_WIDTH, 2.f});
    for (int row = 0; row <= Grid::ROWS_PER_BLOCK; ++row) {
        line.setPosition({Grid::GRID_LEFT, screenY + row * Grid::CELL_SIZE});
        win_.draw(line);
    }
}

void UIManager::drawPlayer()
{
    CharacterRenderer::draw(win_, ctx_.selectedCharacterID,
                            {playerWorldPos_.x, playerWorldPos_.y - cameraY_}, PLAYER_RADIUS);
}

void UIManager::renderPlay()
{
    if (state_ == UIState::EndlessPlay) {
        for (const MapBlock& block : endlessMap_.getBlocks())
            drawMapBlock(block, cameraY_);
    } else {
        for (const MapBlock& block : classicMap_.getBlocks())
            drawMapBlock(block, cameraY_);
    }
    // Draw the test obstacles with camera offset
    for (auto obs : Obstacles) {
        obs->draw(win_, cameraY_);
    }

    drawPlayer();

    // Dedicated, 600px gameplay sidebar.
    sf::RectangleShape sidebar({Grid::SIDEBAR_WIDTH, Grid::MAP_HEIGHT});
    sidebar.setPosition({Grid::MAP_WIDTH, 0.f});
    sidebar.setFillColor(sf::Color(26, 31, 40));
    win_.draw(sidebar);
    sf::RectangleShape divider({4.f, Grid::MAP_HEIGHT});
    divider.setPosition({Grid::MAP_WIDTH - 2.f, 0.f});
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
    timerText.setPosition({Grid::MAP_WIDTH + 72.f, 110.f});
    win_.draw(timerText);

    sf::Text pauseText(font_, "ESC to Pause", 30);
    pauseText.setFillColor(sf::Color(210, 215, 225));
    pauseText.setPosition({Grid::MAP_WIDTH + 72.f, 205.f});
    win_.draw(pauseText);

    sf::Text movementText(font_, gameplayStarted_ ? "Arrow keys: move" : "Press an arrow key to start", 24);
    movementText.setFillColor(sf::Color(170, 180, 195));
    movementText.setPosition({Grid::MAP_WIDTH + 72.f, 265.f});
    win_.draw(movementText);
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
        drawCenteredText("Score: " + std::to_string(ctx_.endlessScore), 360, 22, sf::Color::White);
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
