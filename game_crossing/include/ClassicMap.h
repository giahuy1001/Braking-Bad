#pragma once

#include "EndlessMap.h"
#include <vector>

// A finite, prebuilt map for Classic mode. World Y is negative while moving
// toward the top of the course, matching EndlessMap and the player camera.
class ClassicMap
{
public:
    void init(int level)
    {
        const int count = level <= 5 ? 1 : 2;
        m_blocks.clear();
        m_blocks.reserve(count);
        for (int i = 0; i < count; ++i) {
            MapBlock block;
            block.blockID = static_cast<std::uint32_t>(i);
            block.biome = (i % 2 == 0) ? BiomeType::Urban : BiomeType::Desert;
            block.startY = -EndlessMap::BLOCK_HEIGHT * (i + 1);
            block.endY = -EndlessMap::BLOCK_HEIGHT * i;
            block.lanes = generateLaneLayout(static_cast<std::uint32_t>(level * 100 + i), i == 0);
            m_blocks.push_back(block);
        }
    }

    const std::vector<MapBlock>& getBlocks() const { return m_blocks; }
    float topLimit() const { return m_blocks.empty() ? 0.f : m_blocks.back().startY; }
    float bottomLimit() const { return m_blocks.empty() ? 0.f : m_blocks.front().endY; }

private:
    // Stored bottom-to-top so block 0 is the starting block.
    std::vector<MapBlock> m_blocks;
};
