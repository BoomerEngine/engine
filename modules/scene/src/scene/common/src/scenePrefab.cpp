/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: prefab #]
*
***/

#include "build.h"
#include "scenePrefab.h"
#include "sceneNodeTemplate.h"

#include "base/resources/include/resourceFactory.h"
#include "base/resources/include/resourceSerializationMetadata.h"

namespace scene
{

    ///----

    base::ConfigProperty<float> cvPrefabUpdateDelay("Scene.Prefab", "UpdateDelay", 0.5f);

    ///----

    // factory class for the scene template
    class ScenePrefabFactory : public base::res::IFactory
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ScenePrefabFactory, base::res::IFactory);

    public:
        virtual base::res::ResourceHandle createResource() const override final
        {
            return base::CreateSharedPtr<Prefab>();
        }
    };

    RTTI_BEGIN_TYPE_CLASS(ScenePrefabFactory);
        RTTI_METADATA(base::res::FactoryClassMetadata).bindResourceClass<Prefab>();
    RTTI_END_TYPE();

    ///----

	RTTI_BEGIN_TYPE_CLASS(Prefab);
        RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4prefab");
        RTTI_PROPERTY(m_container);
        RTTI_PROPERTY(m_nodes);
    RTTI_END_TYPE();

    Prefab::Prefab()
        : m_dataVersion(0)
    {
        if (!base::rtti::IClassType::IsDefaultObjectCreation())
        {
            m_container = base::CreateSharedPtr<NodeTemplateContainer>();
            m_container->parent(sharedFromThis());
        }
    }

    Prefab::~Prefab()
    {
    }

    bool Prefab::checkVersion(uint32_t version) const
    {
        auto lock = base::CreateLock(m_containerLock);
        return m_dataVersion == version;
    }

    NodeTemplateContainerPtr Prefab::nodeContainer(uint32_t* outDataVersion) const
    {
        auto lock = base::CreateLock(m_containerLock);

        if (outDataVersion)
            *outDataVersion = m_dataVersion;

        return m_container;
    }

    void Prefab::content(const NodeTemplateContainerPtr& newContent)
    {
        {
            auto lock = base::CreateLock(m_containerLock);
            m_container = newContent;
            m_container->parent(sharedFromThis());
            m_dataVersion += 1;
        }

        markModified();

        postEvent("PrefabChanged"_id);
    }

    void Prefab::onPostLoad()
    {
        TBaseClass::onPostLoad();

        // fill the container with old nodes
        if (!m_nodes.empty())
        {
            for (auto& node : m_nodes)
                m_container->addNode(node, false);
            m_nodes.clear();
        }
    }

    NodeTemplateContainerPtr Prefab::compile(const PrefabCompilationSettings& settings, PrefabDependencies* outDependencies /*= nullptr*/) const
    {
        // snapshot the data to compile from 
        uint32_t dataVersion;
        auto data = nodeContainer(&dataVersion);
        if (!data || data->rootNodes().empty())
            return nullptr;

        auto rootIndex = data->rootNodes()[0]; //  TODO: randomize, select, etc

        auto ret = base::CreateSharedPtr<NodeTemplateContainer>();
        data->compile(rootIndex, settings.m_placement, ret, -1, outDependencies);

        return ret;
    }

} // scene
