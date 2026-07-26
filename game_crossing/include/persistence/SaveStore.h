#pragma once
#include "persistence/PlayerProfile.h"
#include <array>
#include <string>

// ---------------------------------------------------------------------
//  Per-mode save slots.  Max 3 most recent per mode (Classic, Endless).
//  FIFO eviction: when all 3 slots are full, the oldest is dropped.
//
//  Files:
//      saves_classic.txt
//      saves_endless.txt
//
//  Line format (one record per line):
//      name|level|elapsedSec|score|savedAtUnix
//  Any malformed line is skipped on load (graceful degradation).
// ---------------------------------------------------------------------
class SaveStore
{
public:
    static constexpr int kMaxSlots = 3;

    SaveStore();

    std::array<RunRecord, kMaxSlots> slots(GameMode m) const;

    // Pushes a new record.  If all slots are full, slot 0 is dropped
    // and the remaining two shift down, leaving slot 2 for the new one.
    void push(const RunRecord& r);

    void clear(int slotIdx, GameMode m);
    void clearAll(GameMode m);

    // Loads everything from disk (called once from UIManager ctor).
    void loadAll();
    // Writes everything to disk.  Called after every mutation.
    void saveAll() const;

private:
    std::array<RunRecord, kMaxSlots> classic_{};
    std::array<RunRecord, kMaxSlots> endless_{};
    int classicCount_ = 0;
    int endlessCount_ = 0;

    static std::string pathFor(GameMode m);
    static std::string serialize(const RunRecord& r);
    static bool        deserialize(const std::string& line, RunRecord& out);

    std::array<RunRecord, kMaxSlots>& arr(GameMode m);
    int& countOf(GameMode m);
};
