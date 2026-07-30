#include "persistence/SaveStore.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace
{
    constexpr const char* kClassicFile = "saves_classic.txt";
    constexpr const char* kEndlessFile = "saves_endless.txt";
}

SaveStore::SaveStore() = default;

std::string SaveStore::pathFor(GameMode m)
{
    return (m == GameMode::Classic) ? kClassicFile : kEndlessFile;
}

std::array<RunRecord, SaveStore::kMaxSlots>& SaveStore::arr(GameMode m)
{
    return (m == GameMode::Classic) ? classic_ : endless_;
}

int& SaveStore::countOf(GameMode m)
{
    return (m == GameMode::Classic) ? classicCount_ : endlessCount_;
}

std::array<RunRecord, SaveStore::kMaxSlots>
SaveStore::slots(GameMode m) const
{
    auto& a = (m == GameMode::Classic) ? classic_ : endless_;
    int   c = (m == GameMode::Classic) ? classicCount_ : endlessCount_;
    std::array<RunRecord, kMaxSlots> out{};
    for (int i = 0; i < c; ++i) out[i] = a[i];
    std::sort(out.begin(), out.begin() + c, [](const RunRecord& lhs, const RunRecord& rhs) {
        return lhs.savedAtUnix > rhs.savedAtUnix;
    });
    return out;
}

void SaveStore::push(const RunRecord& r)
{
    auto& a = arr(r.mode);
    int& cnt = countOf(r.mode);
    const int cap = kMaxSlots;

    if (cnt >= cap)
    {
        // FIFO eviction: drop slot 0, shift 1->0, 2->1, write new into 2.
        a[0] = a[1];
        a[1] = a[2];
        a[2] = r;
        cnt = cap;
    }
    else
    {
        a[cnt++] = r;
    }
    saveAll();
}

void SaveStore::clear(int slotIdx, GameMode m)
{
    if (slotIdx < 0 || slotIdx >= kMaxSlots) return;
    auto& a = arr(m);
    int& cnt = countOf(m);
    if (slotIdx >= cnt) return;

    for (int i = slotIdx; i < cnt - 1; ++i)
        a[i] = a[i + 1];
    a[cnt - 1] = RunRecord{};
    --cnt;
    saveAll();
}

void SaveStore::clearAll(GameMode m)
{
    auto& a = arr(m);
    int& cnt = countOf(m);
    for (int i = 0; i < cnt; ++i) a[i] = RunRecord{};
    cnt = 0;
    saveAll();
}

std::string SaveStore::serialize(const RunRecord& r)
{
    std::ostringstream os;
    os << r.name << '|'
        << r.level << '|'
        << r.elapsedSec << '|'
        << r.score << '|'
        << r.savedAtUnix << '|'
        // Ghi thêm tọa độ
        << r.playerX << '|'
        << r.playerY << '|'
        << r.cameraY;
    return os.str();
}

bool SaveStore::deserialize(const std::string& line, RunRecord& out)
{
    std::stringstream ss(line);
    std::string name;
    int level = 0, elapsed = 0, score = 0;
    std::int64_t ts = 0;
    char bar;

    if (!std::getline(ss, name, '|')) return false;
    if (!(ss >> level))  return false;
    if (!(ss >> bar))    return false;
    if (!(ss >> elapsed))return false;
    if (!(ss >> bar))    return false;
    if (!(ss >> score))  return false;
    if (!(ss >> bar))    return false;
    if (!(ss >> ts))     return false;

    // Đọc thêm tọa độ (dùng if để tránh lỗi nếu đọc nhầm file save cũ)
    if (ss >> bar) {
        if (!(ss >> out.playerX)) return false;
        if (!(ss >> bar)) return false;
        if (!(ss >> out.playerY)) return false;
        if (!(ss >> bar)) return false;
        if (!(ss >> out.cameraY)) return false;
    }

    out.name = name;
    out.level = level;
    out.elapsedSec = elapsed;
    out.score = score;
    out.savedAtUnix = ts;
    return true;
}
void SaveStore::loadAll()
{
    auto loadMode = [&](GameMode m) {
        auto& a = arr(m);
        int& cnt = countOf(m);
        cnt = 0;
        std::ifstream in(pathFor(m));
        if (!in.is_open()) return;
        std::string line;
        while (std::getline(in, line) && cnt < kMaxSlots)
        {
            RunRecord r;
            if (deserialize(line, r))
            {
                r.mode = m;
                a[cnt++] = r;
            }
        }
        };
    loadMode(GameMode::Classic);
    loadMode(GameMode::Endless);
}

void SaveStore::saveAll() const
{
    auto saveMode = [&](GameMode m) {
        const auto& a = (m == GameMode::Classic) ? classic_ : endless_;
        const int   c = (m == GameMode::Classic) ? classicCount_ : endlessCount_;
        std::ofstream out(pathFor(m), std::ios::trunc);
        if (!out.is_open()) return;
        for (int i = 0; i < c; ++i)
            out << serialize(a[i]) << '\n';
        };
    saveMode(GameMode::Classic);
    saveMode(GameMode::Endless);
}
