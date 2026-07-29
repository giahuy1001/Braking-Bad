#include "EndlessMap.h"

#include "Grid.h"
#include <algorithm>
#include <random>
#include <fstream>
#include <sstream>

std::string biomeToString(BiomeType biome)
{
    switch (biome) {
    case BiomeType::Swamp:  return "Swamp";
    case BiomeType::Urban:  return "Urban";
    case BiomeType::Desert: return "Desert";
    default:                return "Unknown";
    }
}

std::array<LaneType, LANES_PER_BLOCK> generateLaneLayout(std::uint32_t seed,
    bool isStartingBlock)
{
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> chance(0, 99);
    std::uniform_int_distribution<int> hazard(0, 1);
    std::array<LaneType, LANES_PER_BLOCK> lanes{};
    int dangerStreak = 0;

    for (int row = 0; row < LANES_PER_BLOCK; ++row)
    {
        // The starting row is the lower row (index 8) of the first block.
        const bool forceSafe = (isStartingBlock && row == LANES_PER_BLOCK - 1) ||
            dangerStreak >= 3 || chance(rng) < 42;
        if (forceSafe) {
            lanes[row] = LaneType::Safe;
            dangerStreak = 0;
        }
        else {
            lanes[row] = hazard(rng) == 0 ? LaneType::Vehicle : LaneType::Animal;
            ++dangerStreak;
        }
    }
    return lanes;
}

bool loadMapFromFile(const std::string& filename, std::array<LaneType, LANES_PER_BLOCK>& outLanes, std::array<int, LANES_PER_BLOCK>& outManholes, std::array<int, LANES_PER_BLOCK>& outShields)
{
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    // A malformed/short level file must never inherit lane or manhole data
    // from a previous block.
    outLanes.fill(LaneType::Safe);
    outManholes.fill(-1);
    outShields.fill(-1);

    std::string line;
    for (int i = 0; i < LANES_PER_BLOCK && std::getline(file, line); ++i)
    {
        if (line.empty()) continue;

        if (line[0] == 'V') outLanes[i] = LaneType::Vehicle;
        else if (line[0] == 'A') outLanes[i] = LaneType::Animal;
        else if (line[0] == 'S') {
            outLanes[i] = LaneType::Safe;
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string valStr = line.substr(colonPos + 1);
                valStr.erase(0, valStr.find_first_not_of(" \t")); // Xóa dấu cách thừa
                if (!valStr.empty()) {
                    if (valStr[0] == 'K') { // LÀ KHIÊN
                        try {
                            int gridColumn = std::stoi(valStr.substr(1)) - 1;
                            if (gridColumn >= 0 && gridColumn < Grid::COLUMNS)
                                outShields[i] = gridColumn;
                        }
                        catch (...) {}
                    }
                    else { // LÀ CỐNG
                        try {
                            int gridColumn = std::stoi(valStr) - 1;
                            if (gridColumn >= 0 && gridColumn < Grid::COLUMNS)
                                outManholes[i] = gridColumn;
                        }
                        catch (...) {}
                    }
                }
            }
        }
    }
    return true;
}

bool loadMapFromFile(int levelID, std::array<LaneType, LANES_PER_BLOCK>& outLanes, std::array<int, LANES_PER_BLOCK>& outManholes, std::array<int, LANES_PER_BLOCK>& outShields)
{
    return loadMapFromFile("assets/map/map_level_" + std::to_string(levelID) + ".txt", outLanes, outManholes, outShields);
}

BiomeGenerator::BiomeGenerator(std::uint32_t seed)
    : m_rng(seed),
    m_dist(0, static_cast<int>(BiomeType::COUNT) - 1)
{
}

BiomeType BiomeGenerator::next()
{
    int pick = m_dist(m_rng);
    int attempts = 0;
    while (m_lastBiome != BiomeType::COUNT &&
        m_repeatCount >= MAX_REPEAT &&
        static_cast<BiomeType>(pick) == m_lastBiome &&
        attempts++ < 100) {
        pick = m_dist(m_rng);
    }

    const auto selected = static_cast<BiomeType>(pick);
    if (selected == m_lastBiome)
        ++m_repeatCount;
    else {
        m_lastBiome = selected;
        m_repeatCount = 1;
    }
    return selected;
}

EndlessMap::EndlessMap(float blockHeight, std::uint32_t seed)
    : m_biomeGen(seed == 0 ? std::random_device{}() : seed),
    m_blockHeight(blockHeight)
{
}

void EndlessMap::init()
{
    m_blocks.clear();
    m_nextID = 0;
    m_nextEndY = 0.f;
    for (int i = 0; i < TARGET_BLOCKS; ++i)
        spawnBlockAbove();
}

void EndlessMap::reset()
{
    init();
}

void EndlessMap::setAvailableMapLevels(std::vector<int> levels)
{
    levels.erase(std::remove_if(levels.begin(), levels.end(), [](int level) {
        return level < 1 || level > 10;
    }), levels.end());
    std::sort(levels.begin(), levels.end());
    levels.erase(std::unique(levels.begin(), levels.end()), levels.end());
    m_availableMapLevels = std::move(levels);
}

void EndlessMap::update(float cameraY)
{
    // The camera moves up (towards negative Y). Once a lower block is fully
    // below the viewport, discard it and extend the course above the camera.
    while (!m_blocks.empty() &&
        cameraY + BLOCK_HEIGHT <= m_blocks.back().startY) {
        removeLowestBlock();
        spawnBlockAbove();
    }
    while (static_cast<int>(m_blocks.size()) < TARGET_BLOCKS)
        spawnBlockAbove();
}

void EndlessMap::spawnBlockAbove()
{
    MapBlock block;
    block.blockID = m_nextID++;
    block.biome = m_biomeGen.next();
    block.endY = m_nextEndY;
    block.startY = m_nextEndY - m_blockHeight;
    block.lanes.fill(LaneType::Safe);
    block.manholeCols.fill(-1);

    // --- ĐOẠN CODE CHỌN MAP NGẪU NHIÊN MỚI ---
    // Khai báo danh sách 15 map dưới dạng chuỗi ký tự (bao gồm cả các map x.1)
    static const std::vector<std::string> availableMaps = {
        "1", "2", "3", "4", "5",
        "6", "6.1", "7", "7.1", "8",
        "8.1", "9", "9.1", "10", "10.1"
    };

    // Khởi tạo bộ sinh số ngẫu nhiên
    std::random_device rd;
    std::mt19937 localRng(rd());

    // Chọn ngẫu nhiên 1 index từ 0 đến 14 (tương ứng với 15 map)
    std::uniform_int_distribution<std::size_t> dist(0, availableMaps.size() - 1);
    std::string chosenMap = availableMaps[dist(localRng)];

    // Gán tên file ảnh để hệ thống đồ họa vẽ đúng (ví dụ: map_level_6.1)
    block.mapImageKey = "map_level_" + chosenMap;

    // Tải cấu hình làn đường (Xe/Động vật/An toàn) và nắp cống từ file txt tương ứng
    std::string txtFilePath = "assets/map/map_level_" + chosenMap + ".txt";

    // Gọi hàm loadMapFromFile phiên bản nhận tham số std::string
    if (!loadMapFromFile(txtFilePath, block.lanes, block.manholeCols, block.shieldCols)) {
        // Nếu không tìm thấy file txt, sinh cấu hình an toàn mặc định để không crash game
        block.lanes = generateLaneLayout(block.blockID * 2654435761u, block.blockID == 0);
    }
    // -----------------------------------------

    m_nextEndY = block.startY;
    m_blocks.push_front(block);
}

void EndlessMap::removeLowestBlock()
{
    m_blocks.pop_back();
}
