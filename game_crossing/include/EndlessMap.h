#pragma once

#include <cstdint>
#include <array>
#include <deque>
#include <random>
#include <string>

// World-space Y grows downward. Moving "up" therefore decreases Y. A block is
// data-only; rendering is owned by UIManager (or later by the biome renderer).
enum class BiomeType : std::uint8_t {
    Swamp = 0,
    Urban,
    Desert,
    COUNT
};

std::string biomeToString(BiomeType biome);

enum class LaneType : std::uint8_t {
    Safe = 0,
    Vehicle,
    Animal
};

constexpr int LANES_PER_BLOCK = 9;
std::array<LaneType, LANES_PER_BLOCK> generateLaneLayout(std::uint32_t seed,
                                                           bool isStartingBlock);

struct MapBlock {
    std::uint32_t blockID{0};
    BiomeType biome{BiomeType::Swamp};
    float startY{0.f};
    float endY{0.f};
    // Row 0 is the top row, row 8 is the bottom row of the block.
    std::array<LaneType, LANES_PER_BLOCK> lanes{};

    float height() const { return endY - startY; }
    bool contains(float worldY) const { return worldY >= startY && worldY <= endY; }
};

class BiomeGenerator {
public:
    static constexpr int MAX_REPEAT = 2;

    explicit BiomeGenerator(std::uint32_t seed = 42);
    BiomeType next();

private:
    std::mt19937 m_rng;
    std::uniform_int_distribution<int> m_dist;
    BiomeType m_lastBiome{BiomeType::COUNT};
    int m_repeatCount{0};
};

class EndlessMap {
public:
    static constexpr int TARGET_BLOCKS = 4;
    static constexpr float BLOCK_HEIGHT = 1080.f;

    explicit EndlessMap(float blockHeight = BLOCK_HEIGHT, std::uint32_t seed = 0);

    void init();
    void update(float cameraY);
    void reset();

    const std::deque<MapBlock>& getBlocks() const { return m_blocks; }
    float blockHeight() const { return m_blockHeight; }

private:
    void spawnBlockAbove();
    void removeLowestBlock();

    std::deque<MapBlock> m_blocks;
    BiomeGenerator m_biomeGen;
    std::uint32_t m_nextID{0};
    float m_blockHeight{BLOCK_HEIGHT};
    float m_nextEndY{0.f};
};
