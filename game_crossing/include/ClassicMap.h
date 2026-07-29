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
        m_blocks.clear();

        // ==========================================
        // TRƯỜNG HỢP 1: MÀN 1 ĐẾN 5 (Chỉ 1 map)
        // ==========================================
        if (level <= 5)
        {
            m_blocks.reserve(1);

            MapBlock block;
            block.blockID = 0;
            block.mapImageKey = "map_level_" + std::to_string(level);
            block.biome = BiomeType::Urban; // Đảm bảo đúng màu gốc
            block.startY = -EndlessMap::BLOCK_HEIGHT * 1;
            block.endY = 0;

            // Đọc 1 file tương ứng với level
            loadMapFromFile(level, block.lanes, block.manholeCols, block.shieldCols);

            m_blocks.push_back(block);
        }
        // ==========================================
        // TRƯỜNG HỢP 2: MÀN 6 ĐẾN 10 (Nối 2 map)
        // ==========================================
        else
        {
            m_blocks.reserve(2);

            for (int i = 0; i < 2; ++i)
            {
                MapBlock block;
                block.blockID = static_cast<std::uint32_t>(i);
                block.mapImageKey = "map_level_" + std::to_string(level) + (i == 0 ? "" : ".1");

                // Đảm bảo thứ tự màu sắc: Map dưới là Urban, Map trên là Desert
                block.biome = (i == 0) ? BiomeType::Urban : BiomeType::Desert;

                // Tính tọa độ nối map
                block.startY = -EndlessMap::BLOCK_HEIGHT * (i + 1);
                block.endY = -EndlessMap::BLOCK_HEIGHT * i;

                // Xác định ID map cần đọc
                if (i == 0)
                {
                    // Block dưới: Đọc map chính bằng số (ví dụ: map_level_6.txt)
                    loadMapFromFile(level, block.lanes, block.manholeCols, block.shieldCols);
                }
                else
                {
                    // Block trên: Ghép chuỗi để đọc map phụ bằng chữ (ví dụ: map_level_6.1.txt)
                    std::string filename = "assets/map/map_level_" + std::to_string(level) + ".1.txt";
                    loadMapFromFile(filename, block.lanes, block.manholeCols, block.shieldCols);
                }

                m_blocks.push_back(block);
            }
        }
    }
    float topLimit() const { return m_blocks.empty() ? 0.f : m_blocks.back().startY; }
    float bottomLimit() const { return m_blocks.empty() ? 0.f : m_blocks.front().endY; }
    std::vector<MapBlock>& getMutableBlocks() { return m_blocks; }
    const std::vector<MapBlock>& getBlocks() const { return m_blocks; }

private:
    // Stored bottom-to-top so block 0 is the starting block.
    std::vector<MapBlock> m_blocks;
};
