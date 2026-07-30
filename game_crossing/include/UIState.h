/**
 * @file UIState.h
 * @brief Shared UI state, transition context, and modal-layer declarations.
 */
#pragma once

#include <string>

/** @brief Identifies every mutually exclusive screen managed by UIManager. */
enum class UIState : unsigned char
{
    /** @brief Startup loading screen that waits for player confirmation. */
    Loading = 0,
    /** @brief Main navigation screen. */
    MainMenu,
    /** @brief Game-mode selection screen. */
    ModeSelect,
    /** @brief Player-name input screen. */
    NameInput,
    /** @brief Classic-level selection screen. */
    LevelSelect,
    /** @brief Audio settings screen. */
    Setting,
    /** @brief Theme and character selection screen. */
    Graphic,
    /** @brief Saved-run selection screen. */
    LoadGame,
    /** @brief Leaderboard screen. */
    Ranking,
    /** @brief Game instructions screen. */
    Help,
    /** @brief Active classic gameplay screen. */
    ClassicPlay,
    /** @brief Active endless gameplay screen. */
    EndlessPlay,
    /** @brief Post-run results screen. */
    GameOver,
    /** @brief Paused-game overlay state. */
    Pause
};

/** @brief Preserves game-selection and run data across UI state transitions. */
struct StateContext
{
    /** @brief Specifies the game mode associated with a pending or active run. */
    enum class Mode : unsigned char { None, Classic, Endless };

    /** @brief Selected game mode. */
    Mode        mode          = Mode::None;
    /** @brief Selected classic level. */
    int         level         = 0;
    /** @brief Selected character identifier. */
    int         selectedCharacterID = 1;
    /** @brief Validated player name awaiting run creation. */
    std::string pendingName;
    /** @brief Recorded endless-run score. */
    int         endlessScore  = 0;
    /** @brief Recorded endless-run duration in seconds. */
    int         endlessSec    = 0;
    /** @brief Highest classic level reached. */
    int         classicLevel  = 0;
    /** @brief Recorded classic-run duration in seconds. */
    int         classicSec    = 0;
};

/** @brief Identifies an optional dialog layered above the active screen. */
enum class Modal : unsigned char
{
    /** @brief No modal is active. */
    None = 0,
    /** @brief Requests confirmation before leaving the application. */
    ConfirmExit,
    /** @brief Requests confirmation before saving a run. */
    SaveGame,
    /** @brief Requests confirmation before abandoning a run. */
    QuitGame
};
