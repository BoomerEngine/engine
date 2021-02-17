/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
*
***/

#pragma once

#include "base/world/include/worldEntity.h"
#include "base/world/include/worldNodeTemplate.h"

namespace base
{
    namespace world
    {

        //---

        // cell in the streaming grid
        struct BASE_WORLD_COMPILER_API SourceStreamingGridCell
        {
            int cellX = 0;
            int cellY = 0;
            int level = 0;

            Array<RefPtr<SourceIsland>> islands;
        };

        //---

        // cell in the streaming grid
        struct BASE_WORLD_COMPILER_API SourceStreamingGridLevel
        {
            float cellSize = 0.0f;
            Rect cellArea;

            Array<SourceStreamingGridCell> cells;
        };

        //--

        // streaming grid
        struct BASE_WORLD_COMPILER_API SourceStreamingGrid
        {
            float baseCellSize = 0.0f;
            Rect baseCellArea;
            Box streamingArea;

            Array<SourceStreamingGridLevel> levels; // levels - from smallest cell to largest
        };

        //--

        // initialize grid for given total world streaming box
        extern BASE_WORLD_COMPILER_API void InitializeGrid(const Box& totalStreamableArea, float smallestCellSize, float largestStreamingDistance, SourceStreamingGrid& outGrid);

        // insert island into streaming grid
        extern BASE_WORLD_COMPILER_API void InsertIslandIntoGrid(SourceIsland* island, SourceStreamingGrid& outGrid);

        // print stats
        extern BASE_WORLD_COMPILER_API void DumpGrid(const SourceStreamingGrid& grid);

        // collect non empty cells
        extern BASE_WORLD_COMPILER_API void CollectFinalCells(const SourceStreamingGrid& grid, Array<const SourceStreamingGridCell*>& outCells);

        // build island from data
        extern BASE_WORLD_COMPILER_API RefPtr<StreamingIsland> BuildIsland(const SourceIsland* island);

        // build cell data
        //extern BASE_WORLD_COMPILER_API RefPtr<StreamingSector> BuildSectorFromCell(const SourceStreamingGridCell& cell);

        //---

    } // world
} // base
