/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\resources #]
*
***/

#include "build.h"
#include "compiledScene.h"
#include "streamingIsland.h"
#include "core/resource/include/tags.h"

BEGIN_BOOMER_NAMESPACE()

///----

RTTI_BEGIN_TYPE_CLASS(CompiledScene);
    RTTI_METADATA(ResourceExtensionMetadata).extension("v4sectors");
    RTTI_METADATA(ResourceDescriptionMetadata).description("Compiled Scene");
    RTTI_METADATA(ResourceTagColorMetadata).color(0x9d, 0x02, 0x08);
    RTTI_PROPERTY(m_rootIslands);
RTTI_END_TYPE();

CompiledScene::CompiledScene()
{
}

CompiledScene::CompiledScene(Array<StreamingIslandPtr>&& rootIslands)
{
    m_rootIslands = std::move(rootIslands);

    for (const auto& island : m_rootIslands)
        island->parent(this);
}

///----

END_BOOMER_NAMESPACE()
