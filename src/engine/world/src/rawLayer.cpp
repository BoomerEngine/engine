/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\resources #]
*
***/

#include "build.h"
#include "rawLayer.h"
#include "nodeTemplate.h"

#include "core/resource/include/resourceFactory.h"
#include "core/resource/include/resourceTags.h"

BEGIN_BOOMER_NAMESPACE()

///----

RTTI_BEGIN_TYPE_CLASS(RawLayer);
    RTTI_METADATA(res::ResourceExtensionMetadata).extension("v4layer");
    RTTI_METADATA(res::ResourceDescriptionMetadata).description("Scene Layer");
    RTTI_METADATA(res::ResourceTagColorMetadata).color(0xe8, 0x5d, 0x04);
    RTTI_PROPERTY(m_nodes);
RTTI_END_TYPE();

RawLayer::RawLayer()
{
}

void RawLayer::setup(const Array<NodeTemplatePtr>& sourceNodes)
{
    m_nodes.reset();

    for (const auto& node : sourceNodes)
    {
        if (node)
        {
            DEBUG_CHECK(node->parent() == nullptr);
            node->parent(this);
            m_nodes.pushBack(node);
        }
    }
}

///----

END_BOOMER_NAMESPACE()
