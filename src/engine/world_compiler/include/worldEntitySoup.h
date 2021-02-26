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
#include "engine/world/include/worldNodeTemplate.h"

BEGIN_BOOMER_NAMESPACE()

//---

// entity soup
struct ENGINE_WORLD_COMPILER_API SourceEntitySoup : public NoCopy
{
    Array<RefPtr<HierarchyEntity>> rootEntities;
    uint32_t totalEntityCount = 0;
};

//---

// extract source entities from a scene
extern ENGINE_WORLD_COMPILER_API void ExtractSourceEntities(const res::ResourcePath& worldFilePath, SourceEntitySoup& outSoup);

//---

END_BOOMER_NAMESPACE()
