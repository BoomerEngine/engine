/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\resources #]
*
***/

#include "build.h"
#include "compiledWorldData.h"
#include "compiledStreaming.h"
#include "compiledIsland.h"

#include "world.h"
#include "worldParameters.h"

#include "core/resource/include/tags.h"

BEGIN_BOOMER_NAMESPACE()

///----

RTTI_BEGIN_TYPE_CLASS(CompiledWorldData);
    RTTI_METADATA(ResourceDescriptionMetadata).description("Compiled Scene");
    RTTI_PROPERTY(m_rootIslands);
    RTTI_PROPERTY(m_parameters);
RTTI_END_TYPE();

CompiledWorldData::CompiledWorldData()
{
}

CompiledWorldData::CompiledWorldData(Array<CompiledStreamingIslandPtr>&& rootIslands, Array<WorldParametersPtr>&& parameters)
{
    m_rootIslands = std::move(rootIslands);
    m_parameters = std::move(parameters);

    for (const auto& ptr : m_rootIslands)
        ptr->parent(this);

    for (const auto& ptr : m_parameters)
        ptr->parent(this);
}

Array<WorldParametersPtr> CompiledWorldData::compileWorldParameters() const
{
    return m_parameters;
}

///----

WorldPtr World::CreateCompiledWorld(const CompiledWorldData* compiledScene)
{
    DEBUG_CHECK_RETURN_EX_V(compiledScene, "No editable world", nullptr);
    auto compiledStreaming = RefNew<CompiledWorldStreaming>(compiledScene);
    return RefNew<World>(WorldType::Compiled, nullptr, compiledStreaming);
}

///----


END_BOOMER_NAMESPACE()
