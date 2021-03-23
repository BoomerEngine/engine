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
#include "rawEntity.h"

#include "core/resource/include/factory.h"
#include "core/resource/include/tags.h"

BEGIN_BOOMER_NAMESPACE()

///----

RTTI_BEGIN_TYPE_CLASS(RawLayer);
    RTTI_METADATA(ResourceDescriptionMetadata).description("World Layer");
    RTTI_PROPERTY(m_nodes);
RTTI_END_TYPE();

RawLayer::RawLayer()
{
}

void RawLayer::setup(const Array<RawEntityPtr>& sourceNodes)
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
