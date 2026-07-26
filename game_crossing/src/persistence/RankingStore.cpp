#include "persistence/RankingStore.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace
{
    constexpr const char* kClassicFile = "rank_classic.txt";
    constexpr const char* kEndlessFile = "rank_endless.txt";
}

RankingStore::RankingStore() = default;

std::string RankingStore::pathFor(GameMode m)
{
    return (m == GameMode::Classic) ? kClassicFile : kEndlessFile;
}

std::vector<RunRecord>& RankingStore::vec(GameMode m)
{
    return (m == GameMode::Classic) ? classic_ : endless_;
}

const std::vector<RunRecord>& RankingStore::vec(GameMode m) const
{
    return (m == GameMode::Classic) ? classic_ : endless_;
}

// -------- sort comparators (the spec's core algorithmic logic) --------

bool RankingStore::classicLess(const RunRecord& a, const RunRecord& b)
{
    // a should come BEFORE b in the sorted list iff a is BETTER.
    //  1) higher level wins  -> a better iff a.level > b.level
    //  2) tie on level: shorter time wins -> a better iff a.elapsedSec < b.elapsedSec
    //  3) tie on time: earlier savedAtUnix wins -> a better iff a.savedAtUnix < b.savedAtUnix
    //  4) tie: name asc (a better iff a.name < b.name)
    if (a.level        != b.level)        return a.level        >  b.level;
    if (a.elapsedSec   != b.elapsedSec)   return a.elapsedSec   <  b.elapsedSec;
    if (a.savedAtUnix  != b.savedAtUnix)  return a.savedAtUnix  <  b.savedAtUnix;
    return a.name < b.name;
}

bool RankingStore::endlessLess(const RunRecord& a, const RunRecord& b)
{
    //  1) higher score wins
    //  2) tie on score: longer time wins (survived more)
    //  3) tie on time: earlier savedAtUnix wins
    //  4) tie: name asc
    if (a.score        != b.score)        return a.score        >  b.score;
    if (a.elapsedSec   != b.elapsedSec)   return a.elapsedSec   >  b.elapsedSec;
    if (a.savedAtUnix  != b.savedAtUnix)  return a.savedAtUnix  <  b.savedAtUnix;
    return a.name < b.name;
}

// -------- persistence --------

std::string RankingStore::serialize(const RunRecord& r)
{
    std::ostringstream os;
    os << r.name << '|'
       << static_cast<int>(r.mode) << '|'
       << r.level << '|'
       << r.elapsedSec << '|'
       << r.score << '|'
       << r.savedAtUnix;
    return os.str();
}

bool RankingStore::deserialize(const std::string& line, RunRecord& out)
{
    std::stringstream ss(line);
    std::string name;
    int modeI = 0, level = 0, elapsed = 0, score = 0;
    std::int64_t ts = 0;
    char bar;
    if (!std::getline(ss, name, '|'))   return false;
    if (!(ss >> modeI))                 return false;
    if (!(ss >> bar))                   return false;
    if (!(ss >> level))                 return false;
    if (!(ss >> bar))                   return false;
    if (!(ss >> elapsed))               return false;
    if (!(ss >> bar))                   return false;
    if (!(ss >> score))                 return false;
    if (!(ss >> bar))                   return false;
    if (!(ss >> ts))                    return false;

    out.name        = name;
    out.mode        = static_cast<GameMode>(modeI);
    out.level       = level;
    out.elapsedSec  = elapsed;
    out.score       = score;
    out.savedAtUnix = ts;
    return true;
}

void RankingStore::loadAll()
{
    auto loadMode = [&](GameMode m) {
        auto& v = vec(m);
        v.clear();
        std::ifstream in(pathFor(m));
        if (!in.is_open()) return;
        std::string line;
        while (std::getline(in, line))
        {
            RunRecord r;
            if (deserialize(line, r))
            {
                r.mode = m;
                v.push_back(r);
            }
        }
    };
    loadMode(GameMode::Classic);
    loadMode(GameMode::Endless);
}

void RankingStore::saveAll() const
{
    auto saveMode = [&](GameMode m) {
        const auto& v = vec(m);
        std::ofstream out(pathFor(m), std::ios::trunc);
        if (!out.is_open()) return;
        for (const auto& r : v)
            out << serialize(r) << '\n';
    };
    saveMode(GameMode::Classic);
    saveMode(GameMode::Endless);
}

void RankingStore::submit(const RunRecord& r)
{
    auto& v = vec(r.mode);
    v.push_back(r);

    if (r.mode == GameMode::Classic)
        std::sort(v.begin(), v.end(), &RankingStore::classicLess);
    else
        std::sort(v.begin(), v.end(), &RankingStore::endlessLess);

    if (static_cast<int>(v.size()) > kMaxOnDisk)
        v.resize(kMaxOnDisk);

    saveAll();
}

std::vector<RunRecord> RankingStore::all(GameMode m) const
{
    return vec(m);
}

void RankingStore::clear(GameMode m)
{
    vec(m).clear();
    saveAll();
}
