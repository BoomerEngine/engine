/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: command #]
*
***/

#include "build.h"
#include "streamingEntitySoup.h"
#include "streamingIslandGeneration.h"
#include "streamingGrid.h"
#include "worldCompiler.h"

#include "engine/world/include/rawWorldData.h"
#include "engine/world/include/compiledWorldData.h"

BEGIN_BOOMER_NAMESPACE()

static void DistributeIslandsIntoGrid(const SourceIslands& islands, SourceStreamingGrid& outGrid)
{
    for (const auto& island : islands.rootIslands)
        InsertIslandIntoGrid(island, outGrid);
}

CompiledWorldDataPtr CompileRawWorld(StringView depotPath, IProgressTracker& progress)
{
    auto rawScene = LoadResource<RawWorldData>(depotPath);
    if (!rawScene)
    {
        TRACE_ERROR("Unable to load '{}'", depotPath);
        return nullptr;
    }

    //--

    // extract entity source
    SourceEntitySoup soup;
    ExtractSourceEntities(depotPath, soup);

    // build entity islands
    SourceIslands islands;
    ExtractSourceIslands(soup, islands);

    //--
    
    // build final islands
    Array<CompiledStreamingIslandPtr> finalIslands;
    finalIslands.reserve(islands.rootIslands.size());
    for (const auto& sourceRootIsland : islands.rootIslands)
    {
        if (auto finalIsland = BuildIsland(sourceRootIsland))
            finalIslands.pushBack(finalIsland);
    }

    // prepare compiled scene object
    auto parameters = rawScene->parameters();
    return RefNew<CompiledWorldData>(std::move(finalIslands), std::move(parameters));
}

//---

END_BOOMER_NAMESPACE()
