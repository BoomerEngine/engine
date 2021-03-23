/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
*
***/

#pragma once

#include "engine/world/include/worldEntity.h"
#include "engine/world/include/rawEntity.h"

BEGIN_BOOMER_NAMESPACE()

//---

struct SourceEntitySoup;

// island of entities that must be streamed together
class ENGINE_WORLD_COMPILER_API SourceIsland : public IReferencable
{
public:
    struct EntityEntry
    {
        int index = -1;
        int parentIndex = -1;
        RefPtr<HierarchyEntity> data;
    };

    Array<EntityEntry> flatEntities;
    Array<RefPtr<SourceIsland>> children;

    Box localStreamingBox; // streaming box of the island
    Box mergedStreamingBox;  // streaming box of the island and child islands
};

//---

struct ENGINE_WORLD_COMPILER_API SourceIslands
{
    Box totalStreamingArea;
    float largestStreamingDistance = 0.0f;

    Array<RefPtr<SourceIsland>> rootIslands;
};

// extract source islands from source entity soup
extern ENGINE_WORLD_COMPILER_API void ExtractSourceIslands(const SourceEntitySoup& outSoup, SourceIslands& outSourceIslands);

//---

END_BOOMER_NAMESPACE()
