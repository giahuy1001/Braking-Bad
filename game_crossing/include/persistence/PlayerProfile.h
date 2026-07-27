#pragma once
#include <string>
#include <cstdint>

// ---------------------------------------------------------------------
//  Shared value types for the persistence layer.
//
//  RunRecord  - one save slot or one ranking row.
//  GameMode   - Classic (10 levels) vs Endless (high score).
//  GraphicChoice - character + background selection (re-used by SaveStore).
// ---------------------------------------------------------------------

enum class GameMode : unsigned char
{
    Classic = 0,
    Endless = 1
};

struct RunRecord
{
    std::string  name;
    GameMode     mode = GameMode::Classic;
    int          level = 0;     // Classic: highest level reached (1..10)
    int          elapsedSec = 0;     // Classic: total time; Endless: survival time
    int          score = 0;     // Endless: score; Classic: 0
    std::int64_t savedAtUnix = 0;     // tiebreaker + display "saved on"

    float        playerX = 0.f;
    float        playerY = 0.f;
    float        cameraY = 0.f;
};

struct GraphicChoice
{
    int characterId = 0;
    int backgroundId = 0;
};
