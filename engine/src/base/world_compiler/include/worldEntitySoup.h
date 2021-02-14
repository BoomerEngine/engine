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

        // entity soup
        struct BASE_WORLD_COMPILER_API SourceEntitySoup : public NoCopy
        {
            Array<RefPtr<HierarchyEntity>> rootEntities;
            uint32_t totalEntityCount = 0;
        };

        //---

        // extract source entities from a scene
        extern BASE_WORLD_COMPILER_API void ExtractSourceEntities(const depot::DepotStructure& depot, const res::ResourcePath& worldFilePath, SourceEntitySoup& outSoup);

        //---

    } // world
} // base
