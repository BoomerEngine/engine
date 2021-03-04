/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\resources #]
***/

#pragma once

#include "core/resource/include/resource.h"

BEGIN_BOOMER_NAMESPACE()

//---

/// Cooked scene
class ENGINE_WORLD_API CompiledScene : public IResource
{
    RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
    RTTI_DECLARE_VIRTUAL_CLASS(CompiledScene, IResource);

public:
    CompiledScene();
    CompiledScene(Array<StreamingIslandPtr>&& rootIslands);

    // get root streaming islands
    INLINE const Array<StreamingIslandPtr>& rootIslands() const { return m_rootIslands; }

private:
    Array<StreamingIslandPtr> m_rootIslands;
};

//---

END_BOOMER_NAMESPACE()
