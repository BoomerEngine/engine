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
class ENGINE_WORLD_API CompiledWorldData : public IResource, public IWorldParametersSource
{
    RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
    RTTI_DECLARE_VIRTUAL_CLASS(CompiledWorldData, IResource);

public:
    CompiledWorldData();
    CompiledWorldData(Array<CompiledStreamingIslandPtr>&& rootIslands, Array<WorldParametersPtr>&& parameters);

    // get root streaming islands
    INLINE const Array<CompiledStreamingIslandPtr>& rootIslands() const { return m_rootIslands; }

    // compiled world parameters
    INLINE const Array<WorldParametersPtr>& parameters() const { return m_parameters; }

private:
    Array<CompiledStreamingIslandPtr> m_rootIslands;
    Array<WorldParametersPtr> m_parameters;

    virtual Array<WorldParametersPtr> compileWorldParameters() const override final;
};

//---

END_BOOMER_NAMESPACE()
