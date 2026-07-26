#pragma once
#include "persistence/PlayerProfile.h"
#include <string>

// ---------------------------------------------------------------------
//  Audio volume + cosmetic choice (character + background).
//
//  Persisted to a single line in settings.txt:
//      volume|characterId|backgroundId
//
//  load() returns defaults if the file is missing or malformed.
//  save() is best-effort; failures are swallowed (the game still plays).
// ---------------------------------------------------------------------
struct GameSettings
{
    int           volume       = 80;   // 0..100
    GraphicChoice cosmetic;
};

class SettingsStore
{
public:
    SettingsStore();

    GameSettings load() const;
    void         save(const GameSettings& s) const;

    // Convenience helpers used by UIManager.
    static std::string filePath();
};
