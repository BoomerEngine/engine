/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
*
***/

#include "build.h"
#include "worldEntityIslandGeneration.h"
#include "worldEntityStreamingGrid.h"
#include "engine/world/include/worldStreamingIsland.h"

BEGIN_BOOMER_NAMESPACE()

//---

void InitializeGrid(const Box& totalStreamableArea, float smallestCellSize, float largestStreamingDistance, SourceStreamingGrid& outGrid)
{
    // hard limit
    if (smallestCellSize < 16.0f)
        smallestCellSize = 16.0f;

    // calculate the world size in base cells
    Rect baseCellRegion;
    baseCellRegion.min.x = (int)std::floor(totalStreamableArea.min.x / smallestCellSize);
    baseCellRegion.min.y = (int)std::floor(totalStreamableArea.min.y / smallestCellSize);
    baseCellRegion.max.x = (int)std::ceil(totalStreamableArea.max.x / smallestCellSize);
    baseCellRegion.max.y = (int)std::ceil(totalStreamableArea.max.y / smallestCellSize);
    TRACE_INFO("Base cell size: {}", smallestCellSize);
    TRACE_INFO("Base cell region: [{},{}] - [{},{}]", baseCellRegion.min.x, baseCellRegion.min.y, baseCellRegion.max.x, baseCellRegion.max.y);
    TRACE_INFO("Base cell size: {}x{} cells", baseCellRegion.width(), baseCellRegion.height());

    // count how many streaming levels are needed
    uint32_t numLevels = 1;
    {
        float levelSize = smallestCellSize;
        while (levelSize < largestStreamingDistance)
        {
            levelSize *= 2.0f;
            numLevels += 1;
        }

        TRACE_INFO("Grid will have {} levels ({} - {}) to accomodate largest streaming distance of {}",
            numLevels, smallestCellSize, levelSize, largestStreamingDistance);
    }

    // initialize the streaming grid
    outGrid.baseCellSize = smallestCellSize;
    outGrid.baseCellArea = baseCellRegion;
    outGrid.streamingArea = totalStreamableArea;
    outGrid.levels.reserve(numLevels);

    // create streaming levels
    float levelSize = smallestCellSize;
    for (uint32_t i = 0; i < numLevels; ++i)
    {
        auto& level = outGrid.levels.emplaceBack();
        level.cellSize = levelSize * (1 << i);                
        level.cellArea.min.x = (int)std::floor(totalStreamableArea.min.x / level.cellSize);
        level.cellArea.min.y = (int)std::floor(totalStreamableArea.min.y / level.cellSize);
        level.cellArea.max.x = (int)std::ceil(totalStreamableArea.max.x / level.cellSize);
        level.cellArea.max.y = (int)std::ceil(totalStreamableArea.max.y / level.cellSize);

        const auto totalWidth = level.cellArea.width();
        const auto totalHeight = level.cellArea.height();
        const auto totalCellCount = totalWidth * totalHeight;

        TRACE_INFO("Grid level {} cell area: [{},{}] - [{},{}] ({}x{} cells, {} total)",
            i, level.cellArea.min.x, level.cellArea.min.y, level.cellArea.max.x, level.cellArea.max.y,
            totalWidth, totalHeight, totalCellCount);

        level.cells.resize(totalCellCount);

        auto* writeCell = level.cells.typedData();
        for (uint32_t y = 0; y < totalHeight; ++y)
        {
            for (uint32_t x = 0; x < totalWidth; ++x, ++writeCell)
            {
                writeCell->cellX = level.cellArea.min.x + x;
                writeCell->cellY = level.cellArea.min.y + y;
                writeCell->level = i;
            }
        }
    }
}

static uint32_t FindLevelForStreamingBox(const SourceStreamingGrid& grid, const Box& box)
{
    // streaming size
    const auto streamingSize = box.extents();
    const auto streamingSizeExt = std::max<float>(streamingSize.x, streamingSize.y);

    // find level that can accommodate the island based on the island streaming size
    uint32_t level = 0;
    while (level < grid.levels.size())
    {
        const auto levelSize = grid.levels[level].cellSize;
        if (streamingSizeExt <= levelSize)
            break;

        ++level;
    }
            
    return level;
}

uint32_t FindBestCellInStreamingGridLevel(const SourceStreamingGrid& grid, uint32_t levelIndex, const Vector3& center)
{
    const auto& level = grid.levels[levelIndex];

    int cellX = (int)std::round(center.x / level.cellSize);
    int cellY = (int)std::round(center.y / level.cellSize);

    cellX = std::clamp<int>(cellX, level.cellArea.min.x, level.cellArea.max.x - 1);
    cellY = std::clamp<int>(cellY, level.cellArea.min.y, level.cellArea.max.y - 1);

    uint32_t index = (cellX - level.cellArea.min.x);
    index += (cellY - level.cellArea.min.y) * level.cellArea.width();
    ASSERT(index < level.cells.size());
    return index;
}

void InsertIslandIntoGrid(SourceIsland* island, SourceStreamingGrid& outGrid)
{
    const auto center = island->mergedStreamingBox.center(); // TODO: better heuristic

    const auto levelIndex = FindLevelForStreamingBox(outGrid, island->mergedStreamingBox);
    const auto cellIndex = FindBestCellInStreamingGridLevel(outGrid, levelIndex, center);

    auto& cell = outGrid.levels[levelIndex].cells[cellIndex];
    cell.islands.pushBack(AddRef(island));
}

//--

void CollectFinalCells(const SourceStreamingGrid& grid, Array<const SourceStreamingGridCell*>& outCells)
{
    for (const auto& level : grid.levels)
        for (const auto& cell : level.cells)
            if (!cell.islands.empty())
                outCells.pushBack(&cell);
}

//--

static uint32_t CountTotalEntities(const SourceIsland* island)
{
    uint32_t ret = island->flatEntities.size();

    for (const auto& child : island->children)
        ret += CountTotalEntities(child);

    return ret;
}

void DumpGrid(const SourceStreamingGrid& grid)
{
    uint32_t levelIndex = 0;
    uint32_t numTotalNonEmptyCells = 0;
    uint32_t numTotalCells = 0;
    for (const auto& level : grid.levels)
    {
        uint32_t numNonEmptyCells = 0;
        for (const auto& cell : level.cells)
        {
            if (!cell.islands.empty())
            {
                uint32_t totalEntities = 0;
                for (const auto& island : cell.islands)
                    totalEntities += CountTotalEntities(island);

                TRACE_INFO("Level {}, cell {}x{}: {} islands, {} entities",
                    cell.level, cell.cellX, cell.cellY,
                    cell.islands.size(), totalEntities);

                numTotalNonEmptyCells += 1;
                numNonEmptyCells += 1;
            }
        }

        if (numNonEmptyCells > 0)
        {
            TRACE_INFO("Level {}: {} non emtpy cells (of of {})", levelIndex, numNonEmptyCells, level.cells.size());
        }

        numTotalCells += level.cells.size();
        levelIndex += 1;
    }

    TRACE_INFO("Grid contains {} non empty cells out of {} ({}%)",
        numTotalNonEmptyCells, numTotalCells,
        Prec(100.0f * (numTotalNonEmptyCells / (float)numTotalCells), 2));
}

//---

RefPtr<StreamingIsland> BuildIsland(const SourceIsland* island)
{
    StreamingIsland::Setup setup;
    setup.streamingBox = island->localStreamingBox;
    //setup.alwaysLoaded = island->alwaysLoaded;

    for (const auto& entity : island->flatEntities)
    {
        auto& entry = setup.entities.emplaceBack();
        entry.id = entity->id;
        entry.data = entity->entity;
    }

    auto ret = RefNew<StreamingIsland>(setup);

    for (const auto& child : island->children)
        if (auto childEntry = BuildIsland(child))
            ret->attachChild(childEntry);

    return ret;
}

/*RefPtr<StreamingSector> BuildSectorFromCell(const SourceStreamingGridCell& cell)
{
    StreamingSector::Setup setup;

    for (const auto& island : cell.islands)
        if (const auto data = BuildIsland(island))
            setup.islands.pushBack(data);

    return RefNew<StreamingSector>(setup);
}*/

//---

END_BOOMER_NAMESPACE()
