#include "EndlessMap.h"

#include <random>

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
        } else {
            lanes[row] = hazard(rng) == 0 ? LaneType::Vehicle : LaneType::Animal;
            ++dangerStreak;
        }
    }
    return lanes;
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
    block.lanes = generateLaneLayout(block.blockID * 2654435761u, block.blockID == 0);
    m_nextEndY = block.startY;
    m_blocks.push_front(block);
}

void EndlessMap::removeLowestBlock()
{
    m_blocks.pop_back();
}
