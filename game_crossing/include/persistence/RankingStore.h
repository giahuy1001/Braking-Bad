#pragma once
#include "persistence/PlayerProfile.h"
#include <vector>
#include <string>

// ---------------------------------------------------------------------
//  Per-mode leaderboard.  Up to 100 entries on disk per mode, top 10
//  drawn at any one time (UIManager handles the scroll viewport).
//
//  Files:
//      rank_classic.txt
//      rank_endless.txt
//
//  Sort rules (the spec's "core algorithmic logic"):
//
//    Classic : higher level wins; on tie, shorter time wins;
//              on tie, earlier savedAtUnix wins; on tie, name asc.
//    Endless : higher score wins; on tie, longer time wins;
//              on tie, earlier savedAtUnix wins; on tie, name asc.
//
//  Both comparators are strict weak orderings (transitive, antisymmetric,
//  irreflexive) so std::sort is well-defined and stable enough for ties.
//  Complexity: O(n log n) per submit on <= 100 entries.
// ---------------------------------------------------------------------
class RankingStore
{
public:
    static constexpr int kMaxOnDisk  = 100;
    static constexpr int kMaxVisible = 10;

    RankingStore();

    void loadAll();
    void saveAll() const;

    // Inserts r, resorts, trims to kMaxOnDisk, persists.
    void submit(const RunRecord& r);

    // Returns the full sorted list (size <= kMaxOnDisk).
    std::vector<RunRecord> all(GameMode m) const;

    void clear(GameMode m);

private:
    std::vector<RunRecord> classic_;
    std::vector<RunRecord> endless_;

    static std::string pathFor(GameMode m);
    static std::string serialize(const RunRecord& r);
    static bool        deserialize(const std::string& line, RunRecord& out);

    static bool classicLess(const RunRecord& a, const RunRecord& b);
    static bool endlessLess(const RunRecord& a, const RunRecord& b);

    std::vector<RunRecord>& vec(GameMode m);
    const std::vector<RunRecord>& vec(GameMode m) const;
};
