/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
*
***/

#include "build.h"
#include "sceneLayer.h"
#include "sceneNodeTemplate.h"

#include "base/resources/include/resourceFactory.h"
#include "base/resources/include/resourceSerializationMetadata.h"

namespace scene
{

    ///----

    // factory class for the scene layers
    class SceneLayerFactory : public base::res::IFactory
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneLayerFactory, base::res::IFactory);

    public:
        virtual base::res::ResourceHandle createResource() const override final
        {
            return base::CreateSharedPtr<Layer>();
        }
    };

    RTTI_BEGIN_TYPE_CLASS(SceneLayerFactory);
        RTTI_METADATA(base::res::FactoryClassMetadata).bindResourceClass<Layer>();
    RTTI_END_TYPE();

    ///----

	RTTI_BEGIN_TYPE_CLASS(Layer);
        RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4layer");
        RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("World Layer");
        RTTI_PROPERTY(m_nodes);
        RTTI_PROPERTY(m_container);
        RTTI_OLD_NAME("scene::world::Layer");
    RTTI_END_TYPE();

    Layer::Layer()
    {
        if (!base::rtti::IClassType::IsDefaultObjectCreation())
        {
            m_container = base::CreateSharedPtr<NodeTemplateContainer>();
            m_container->parent(sharedFromThis());
        }
    }

    Layer::~Layer()
    {
    }

    void Layer::content(const NodeTemplateContainerPtr& container)
    {
        m_container = container;
        m_container->parent(sharedFromThis());
        markModified();
    }

    void Layer::onPostLoad()
    {
        TBaseClass::onPostLoad();

        if (!m_container)
        {
            TRACE_WARNING("No node container found in layer '{}'");
            m_container = base::CreateSharedPtr<NodeTemplateContainer>();
            m_container->parent(sharedFromThis());
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

} // scene
