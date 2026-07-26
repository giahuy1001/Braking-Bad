#pragma once
#include <string>

// ---------------------------------------------------------------------
//  Persistent state about which Classic levels the player has completed.
//  Used by LevelSelect to draw the "tick" mark and to gate which
//  levels are clickable.
//
//  Pass 1: file-backed on disk.  File: progress_classic.txt
//          Single integer 1..10 representing the highest level the
//          player has unlocked.  Level N is "unlocked" iff N <= value.
//          0 means no progress yet (only level 1 unlocked).
//  Pass 2: extend with per-level completion times and per-level
//          check-mark booleans.
// ---------------------------------------------------------------------
class ProgressStore
{
public:
    ProgressStore();

    int  highestUnlockedLevel() const;       // returns 1..10, never 0
    void setHighestUnlockedLevel(int lvl);   // clamps to [1,10]

    void load();
    void save() const;

private:
    int  level_ = 1;
    static std::string filePath();
};
