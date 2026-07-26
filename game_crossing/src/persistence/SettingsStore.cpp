#include "persistence/SettingsStore.h"
#include <fstream>
#include <sstream>

namespace
{
    constexpr const char* kFile = "settings.txt";
}

SettingsStore::SettingsStore() = default;

std::string SettingsStore::filePath()
{
    return kFile;
}

GameSettings SettingsStore::load() const
{
    GameSettings s;
    std::ifstream in(kFile);
    if (!in.is_open())
        return s;

    std::string line;
    if (!std::getline(in, line))
        return s;

    std::stringstream ss(line);
    char bar;
    if (!(ss >> s.volume))            return s;
    if (!(ss >> bar))                 return s;
    if (!(ss >> s.cosmetic.characterId))  return s;
    if (!(ss >> bar))                 return s;
    if (!(ss >> s.cosmetic.backgroundId)) return s;
    return s;
}

void SettingsStore::save(const GameSettings& s) const
{
    std::ofstream out(kFile, std::ios::trunc);
    if (!out.is_open())
        return;
    out << s.volume << '|'
        << s.cosmetic.characterId  << '|'
        << s.cosmetic.backgroundId << '\n';
}
