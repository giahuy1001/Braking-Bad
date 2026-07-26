#include "persistence/ProgressStore.h"
#include <fstream>
#include <algorithm>

namespace { constexpr const char* kFile = "progress_classic.txt"; }

ProgressStore::ProgressStore() = default;

std::string ProgressStore::filePath() { return kFile; }

int ProgressStore::highestUnlockedLevel() const
{
    return level_ < 1 ? 1 : level_;
}

void ProgressStore::setHighestUnlockedLevel(int lvl)
{
    level_ = std::clamp(lvl, 1, 10);
    save();
}

void ProgressStore::load()
{
    std::ifstream in(kFile);
    if (!in.is_open()) return;
    int v = 0;
    if (in >> v) level_ = std::clamp(v, 1, 10);
}

void ProgressStore::save() const
{
    std::ofstream out(kFile, std::ios::trunc);
    if (!out.is_open()) return;
    out << level_ << '\n';
}
