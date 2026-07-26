#pragma once

// ---------------------------------------------------------------------
//  UI state machine for the whole game.
//
//  Replaces the original 5-state skeleton (Menu/Classic/Endless/GameOver/Quit).
//  Every visible screen — including the modal Exit prompt — is a value here
//  except the modal, which is layered on top via UIManager::modal_.
//
//  Transition table is documented in the design notes (see plan §2).
// ---------------------------------------------------------------------
enum class UIState : unsigned char
{
    Boot = 0,             // logo + team name, auto-advance after timer
    MainMenu,             // New Game / Load / Ranking / Setting / Graphic / Help / Exit
    ModeSelect,           // Classic vs Endless (entry from "New Game")
    NameInput,            // asks for player name, validates per rules
    LevelSelect,          // Classic only: 10 tiles with tick marks
    Setting,              // volume slider
    Graphic,              // character + background selectors
    LoadGame,             // 3 slots x 2 modes
    Ranking,              // top 10 leaderboard, 2 tabs
    Help,                 // static instructions
    ClassicPlay,          // gameplay (owned by gameplay team)
    EndlessPlay,          // gameplay
    GameOver,             // post-run summary
    Pause                 // optional, exposed for future use
};

// Carried between state transitions so we don't lose context when navigating.
struct StateContext
{
    enum class Mode : unsigned char { None, Classic, Endless };

    Mode        mode          = Mode::None;
    int         level         = 0;     // 1..10 for ClassicPlay
    std::string pendingName;           // captured by NameInput
    int         endlessScore  = 0;     // written into ranking on GameOver
    int         endlessSec    = 0;
    int         classicLevel  = 0;     // highest reached, written on GameOver
    int         classicSec    = 0;     // total time spent, written on GameOver
};

// Modal layers drawn on top of the current state.
enum class Modal : unsigned char
{
    None = 0,
    ConfirmExit
};