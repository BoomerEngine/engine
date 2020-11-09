/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\resources #]
*
***/

#include "build.h"
#include "worldLayer.h"
#include "worldNodeTemplate.h"

#include "base/resource/include/resourceFactory.h"
#include "base/resource/include/resourceTags.h"

namespace base
{
    namespace world
    {

        ///----

        RTTI_BEGIN_TYPE_CLASS(Layer);
        RTTI_METADATA(res::ResourceExtensionMetadata).extension("v4layer");
        RTTI_METADATA(res::ResourceDescriptionMetadata).description("World Layer");
        RTTI_PROPERTY(m_nodes);
        RTTI_END_TYPE();

        Layer::Layer()
        {
        }

        Layer::Layer(const Array<NodeTemplatePtr>& sourceNodes)
        {
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

    } // world
} // base
