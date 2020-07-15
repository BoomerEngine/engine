/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\resources #]
*
***/

#include "build.h"
#include "worldPrefab.h"
#include "worldNodeTemplate.h"

#include "base/resource/include/resourceFactory.h"
#include "worldNodePath.h"
#include "base/resource/include/resourceTags.h"

namespace game
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
    RTTI_END_TYPE();

    Prefab::Prefab()
    {
        if (!base::IsDefaultObjectCreation())
        {
            m_container = base::CreateSharedPtr<NodeTemplateContainer>();
            m_container->parent(this);
        }
    }

    Prefab::Prefab(const NodeTemplateContainerPtr& existingContainer)
        : m_container(existingContainer)
    {
        DEBUG_CHECK_EX(m_container, "Constructing data prefab with no data");

        if (!m_container)
            m_container = base::CreateSharedPtr<NodeTemplateContainer>();

        m_container->parent(this);
    }

    Prefab::~Prefab()
    {
    }

    NodeTemplateCompiledOutputPtr Prefab::compile(const PrefabCompilationSettings& settings) const
    {
        auto ret = base::CreateSharedPtr<NodeTemplateCompiledOutput>();

        if (ret)
        {
            auto rootIndex = m_container->rootNodes()[0]; //  TODO: randomize, select, etc
            m_container->compileNode(rootIndex, "Node"_id, *ret, nullptr);
        }

        return ret;
    }

    //--

    EntityPtr Prefab::createEntities(base::Array<EntityPtr>& outAllEntities, const base::AbsoluteTransform& placement, const PrefabCompilationSettings& settings /*= PrefabCompilationSettings()*/) const
    {
        // can't spawn from empty prefab
        if (!m_container || m_container->rootNodes().empty())
            return nullptr;

        // compile nodes
        auto compiledNodes = base::CreateSharedPtr<NodeTemplateCompiledOutput>();
        {
            auto rootIndex = m_container->rootNodes()[0]; //  TODO: randomize, select, etc
            m_container->compileNode(rootIndex, "Node"_id, *compiledNodes, nullptr);
        }

        NodeTemplateCreatedEntities entities;
        compiledNodes->createSingleRoot(0, game::NodePath(), placement, entities);
        if (entities.rootEntities.empty())
            return nullptr;
        
        outAllEntities = std::move(entities.allEntities);
        return entities.rootEntities[0];
    }


    //--

} // game
