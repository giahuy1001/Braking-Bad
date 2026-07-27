#pragma once
#include "persistence/PlayerProfile.h"
#include <string>

// ---------------------------------------------------------------------
//  Audio volume + cosmetic choice (character + background).
//
//  Persisted to a single line in settings.txt:
//      sfxVolume|musicVolume|characterId|backgroundId
//
//  load() returns defaults if the file is missing or malformed.
//  save() is best-effort; failures are swallowed (the game still plays).
// ---------------------------------------------------------------------
struct GameSettings
{
    int           volume       = 80;   // SFX volume, 0..100 (kept name for compatibility)
    int           musicVolume  = 80;   // Music volume, 0..100
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
