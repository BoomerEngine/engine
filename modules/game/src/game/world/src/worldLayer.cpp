/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\data #]
*
***/

#include "build.h"
#include "worldLayer.h"
#include "worldNodeTemplate.h"
#include "worldNodeContainer.h"

#include "base/resource/include/resourceFactory.h"
#include "base/resource/include/resourceTags.h"

namespace game
{

    ///----

	RTTI_BEGIN_TYPE_CLASS(WorldLayer);
        RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4layer");
        RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("World Layer");
        RTTI_PROPERTY(m_container);
        RTTI_OLD_NAME("scene::world::WorldLayer");
    RTTI_END_TYPE();

    WorldLayer::WorldLayer()
    {
        if (!base::IsDefaultObjectCreation())
        {
            m_container = base::CreateSharedPtr<NodeTemplateContainer>();
            m_container->parent(this);
        }
    }

    WorldLayer::~WorldLayer()
    {
    }

    void WorldLayer::content(const NodeTemplateContainerPtr& container)
    {
        m_container = container;
        m_container->parent(this);
        markModified();
    }

    void WorldLayer::onPostLoad()
    {
        TBaseClass::onPostLoad();

        if (!m_container)
        {
            TRACE_WARNING("No node container found in layer '{}'");
            m_container = base::CreateSharedPtr<NodeTemplateContainer>();
            m_container->parent(this);
        }

        if (!m_nodes.empty())
        {
            TRACE_INFO("Converting old format of layer '{}' ({} nodes)", path(), m_nodes.size());

            for (auto& node : m_nodes)
                m_container->addNode(node, false);
            m_nodes.clear();
        }
        else
        {
            TRACE_INFO("Loaded node container in layer '{}' with {} nodes ({} roots)", path(), m_container->nodes().size(), m_container->rootNodes().size());
        }
    }

    ///----

} // game
