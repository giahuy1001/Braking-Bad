#pragma once

// Fixed gameplay geometry in physical pixels (the gameplay view is 1920x1080).
namespace Grid
{
    constexpr float MAP_WIDTH = 1320.f;
    constexpr float MAP_HEIGHT = 1080.f;
    constexpr float SIDEBAR_WIDTH = 600.f;
    constexpr float CELL_SIZE = 120.f;
    // Obstacles occupy all 11 columns. Only columns 2..8 are walkable.
    constexpr int COLUMNS = 11;
    constexpr int PLAYABLE_FIRST_COLUMN = 2;
    constexpr int PLAYABLE_LAST_COLUMN = 8;
    constexpr int PLAYABLE_COLUMNS = PLAYABLE_LAST_COLUMN - PLAYABLE_FIRST_COLUMN + 1;
    constexpr int ROWS_PER_BLOCK = 9;
    constexpr float GRID_WIDTH = COLUMNS * CELL_SIZE;             // 1320px
    constexpr float GRID_LEFT = 0.f;
    constexpr float GRID_RIGHT = GRID_LEFT + GRID_WIDTH;

    constexpr float columnCenter(int column)
    {
        return GRID_LEFT + (column + 0.5f) * CELL_SIZE;
    }

    // Map files and MapBlock rows are stored top-to-bottom, zero-based.
    constexpr float rowCenter(float blockStartY, int row)
    {
        return blockStartY + (row + 0.5f) * CELL_SIZE;
    }

    constexpr bool isPlayableColumn(int column)
    {
        return column >= PLAYABLE_FIRST_COLUMN && column <= PLAYABLE_LAST_COLUMN;
    }

    constexpr float playableLeftCenter()  { return columnCenter(PLAYABLE_FIRST_COLUMN); }
    constexpr float playableRightCenter() { return columnCenter(PLAYABLE_LAST_COLUMN); }
}
