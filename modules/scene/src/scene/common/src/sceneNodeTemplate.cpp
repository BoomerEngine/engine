/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: content #]
*
***/

#include "build.h"
#include "scenePrefab.h"
#include "sceneEntity.h"
#include "sceneComponent.h"
#include "sceneNodeTemplate.h"

namespace scene
{

    //--

    RTTI_BEGIN_TYPE_CLASS(ComponentDataTemplate);
        RTTI_PROPERTY(m_enabled);
        RTTI_PROPERTY(m_name);
        RTTI_PROPERTY(m_componentClass);
    RTTI_END_TYPE();
        
    ComponentDataTemplate::ComponentDataTemplate()
        : m_enabled(true)
    {}

    void ComponentDataTemplate::componentClass(base::SpecificClassType<Component> componentClass)
    {
        if (!componentClass || (!componentClass->isAbstract() && componentClass->is(Component::GetStaticClass())))
        {
            m_componentClass = componentClass;
            markModified();
        }
    }

    void ComponentDataTemplate::name(base::StringID name)
    {
        if (m_name != name)
        {
            m_name = name;
            markModified();
        }
    }

    ComponentPtr ComponentDataTemplate::createComponent() const
    {
        if (m_componentClass.empty())
            return nullptr;

        auto ret = m_componentClass->createSharedPtr<Component>();
        applyProperties(ret);
        return ret;
    }

    ComponentDataTemplatePtr ComponentDataTemplate::Merge(const ComponentDataTemplate** templatesToMerge, uint32_t count)
    {
        auto ret = base::CreateSharedPtr<ComponentDataTemplate>();
        ret->m_componentClass = Component::GetStaticClass();

        base::InplaceArray<const base::Array<base::ObjectTemplateParam>*, 32> paramLists;

        for (uint32_t i = 0; i < count; ++i)
        {
            auto t  = templatesToMerge[i];
            if (t->m_componentClass && t->m_componentClass->is(ret->m_componentClass))
                ret->m_componentClass = t->m_componentClass;

            if (!t->m_parameters.empty())
                paramLists.pushBack(&t->m_parameters);
        }


        ObjectTemplate::MergeProperties(paramLists.typedData(), paramLists.size(), ret->m_parameters, ret);

        return ret;
    }

    base::ClassType ComponentDataTemplate::rootTemplateClass() const
    {
        return Component::GetStaticClass();
    }

    base::ClassType ComponentDataTemplate::objectClass() const
    {
        return m_componentClass;
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(EntityDataTemplate);
        RTTI_CATEGORY("Template");
        RTTI_PROPERTY(m_entityClass).editable();
        RTTI_PROPERTY(m_enabled);
    RTTI_END_TYPE();

    EntityDataTemplate::EntityDataTemplate()
        : m_enabled(true)
        , m_entityClass(StaticEntity::GetStaticClass())
    {}

    void EntityDataTemplate::entityClass(base::SpecificClassType<Entity> entityClass)
    {
        if (!entityClass || (!entityClass->isAbstract() && entityClass->is(Entity::GetStaticClass())))
        {
            m_entityClass = entityClass;
            markModified();
        }
    }

    base::ClassType EntityDataTemplate::rootTemplateClass() const
    {
        return Entity::GetStaticClass();
    }

    base::ClassType EntityDataTemplate::objectClass() const
    {
        return m_entityClass;
    }

    base::RefPtr<Entity> EntityDataTemplate::compile() const
    {
        auto entityClass  = m_entityClass;
        if (!entityClass || !entityClass->is(Entity::GetStaticClass()) || entityClass->isAbstract())
            entityClass = StaticEntity::GetStaticClass();

        auto ret = entityClass->createSharedPtr<Entity>();
        applyProperties(ret);
        return ret;
    }

    EntityDataTemplatePtr EntityDataTemplate::Merge(const EntityDataTemplate** templatesToMerge, uint32_t count)
    {
        if (!templatesToMerge || !count)
            return nullptr;

        auto ret = base::CreateSharedPtr<EntityDataTemplate>();
        ret->m_entityClass = Entity::GetStaticClass();

        base::InplaceArray<const base::Array<base::ObjectTemplateParam>*, 32> paramLists;

        for (uint32_t i = 0; i < count; ++i)
        {
            auto t  = templatesToMerge[i];
            if (t->m_entityClass && t->m_entityClass->is(ret->m_entityClass))
                ret->m_entityClass = t->m_entityClass;

            if (!t->m_parameters.empty())
                paramLists.pushBack(&t->m_parameters);
        }

        ObjectTemplate::MergeProperties(paramLists.typedData(), paramLists.size(), ret->m_parameters, ret);

        return ret;
    }

    //--

    RTTI_BEGIN_TYPE_STRUCT(NodeTemplatePrefabSetup);
        RTTI_PROPERTY(m_enabled);
        RTTI_PROPERTY(m_prefab);
    RTTI_END_TYPE();

    //--

    RTTI_BEGIN_TYPE_ENUM(StreamingModel);
        RTTI_ENUM_OPTION(Auto);
        RTTI_ENUM_OPTION(StreamWithParent);
        RTTI_ENUM_OPTION(HierarchicalGrid);
        RTTI_ENUM_OPTION(AlwaysLoaded);
        RTTI_ENUM_OPTION(SeparateSector);
        RTTI_ENUM_OPTION(Discard);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_CLASS(NodeTemplate);
        RTTI_CATEGORY("Node");
        RTTI_PROPERTY(m_name).editable("Name of the node");
        RTTI_PROPERTY(m_placement).editable();
        RTTI_CATEGORY("Streaming");
        RTTI_PROPERTY(m_streamingModel).editable("How the node should be streamed in game");
        RTTI_PROPERTY(m_streamingDistanceOverride).editable("Custom maximum visibility distance in meters for the node");

        RTTI_PROPERTY(m_prefabAssets);
        RTTI_PROPERTY(m_entityTemplate);
        RTTI_PROPERTY(m_componentTemplates);
        RTTI_PROPERTY(m_relativePosition).transient();
        RTTI_PROPERTY(m_relativeRotation).transient();
        RTTI_PROPERTY(m_relativeScale).transient();
    RTTI_END_TYPE();

    /*RTTI_PROPERTY(m_relativePosition).editable("Position relative to parent").units("m").drag(2).noHash();
    RTTI_PROPERTY(m_relativeRotation).editable("Rotation relative to parent").units("°").drag(-360.0f, 360.0f, 2, true).noHash();
    RTTI_PROPERTY(m_relativeScale).editable("Scale relative to parent").noHash();*/

    NodeTemplate::NodeTemplate()
        : m_relativePosition(0,0,0)
        , m_relativeRotation(0,0,0)
        , m_relativeScale(1,1,1)
        , m_streamingDistanceOverride(0)
        , m_streamingModel(StreamingModel::Auto)
    {}

    NodeTemplate::~NodeTemplate()
    {}

    void NodeTemplate::name(base::StringID name)
    {
        if (m_name != name)
        {
            m_name = name;
            onPropertyChanged("name");
        }
    }

    EntityPtr NodeTemplate::compile() const
    {
        // no entity template
        if (!m_entityTemplate)
            return nullptr;

        // compile entity template
        auto entity = m_entityTemplate->compile();
        if (!entity)
            return nullptr;

        // create components
        for (auto& compTemplate : m_componentTemplates)
        {
            if (auto comp = compTemplate->createComponent())
            {
                comp->parent(entity);
                entity->attachComponent(comp);
            }
        }

        return entity;
    }

    NodeTemplatePtr NodeTemplate::convertLegacyContent() const
    {
        return nullptr;
    }

    /*bool NodeTemplate::initializeFromBuildData(const NodeTemplateInitData& data)
    {
        if (data.m_resourcePath.empty())
        {
            if (auto className  = cls()->findMetadata<NodeTemplateClassName>())
            {
                name = base::StringID(className->name());
            }
            else
            {
                name = base::StringID(cls()->name().view().afterLastOrFull("::"));
            }
        }
        else
        {
            name = base::StringID(data.m_resourcePath.path().view().afterLastOrFull("/").beforeFirstOrFull("."));
        }

        placement = data.placement;
        return true;
    }*/
    
    void NodeTemplate::placement(const NodeTemplatePlacement& placement)
    {
        if (m_placement != placement)
        {
            m_placement = placement;
            markModified();
        }
    }

    void NodeTemplate::entityTemplate(const EntityDataTemplatePtr& data)
    {
        if (m_entityTemplate != data)
        {
            m_entityTemplate = data;
            if (m_entityTemplate)
                m_entityTemplate->parent(sharedFromThis());
            markModified();
        }
    }

    void NodeTemplate::addComponentTemplate(const ComponentDataTemplatePtr& data)
    {
        if (data)
        {
            data->parent(sharedFromThis());
            m_componentTemplates.pushBackUnique(data);
            markModified();
        }
    }

    void NodeTemplate::removeComponentTemplate(const ComponentDataTemplatePtr& data)
    {
        if (m_componentTemplates.contains(data))
        {
            m_componentTemplates.remove(data);
            markModified();
        }
    }

    /*base::Transform NodeTemplate::calcLocalToParent() const
    {
        base::Transform ret;
        ret.translation(m_relativePosition);
        ret.scale(m_relativeScale);
        ret.rotation(m_relativeRotation.toQuat());
        return ret;
    }

    base::AbsoluteTransform NodeTemplate::calcLocalToWorld() const
    {
        base::AbsoluteTransform parentTransform;

        if (auto parent = base::rtti_cast<NodeTemplate>(parent.lock()))
            parentTransform = parent->calcLocalToWorld();

        parentTransform.apply(calcLocalToParent());
        return parentTransform;
    }*/

    void NodeTemplate::onPostLoad()
    {
        TBaseClass::onPostLoad();

        if (m_placement.T.isNearZero() && m_placement.R.isNearZero())
        {
            m_placement.T = m_relativePosition;
            m_placement.R = m_relativeRotation;
            m_placement.S = m_relativeScale;
        }
    }

    void NodeTemplate::onPropertyChanged(base::StringView<char> path)
    {
        TBaseClass::onPropertyChanged(path);
    }

    static bool IsScaleReasonable(const base::Vector3& scale)
    {
        auto absScale = scale.abs();
        if (absScale.x < 0.01f || absScale.x > 100.0f)
            return false;
        if (absScale.y < 0.01f || absScale.y > 100.0f)
            return false;
        if (absScale.z < 0.01f || absScale.z > 100.0f)
            return false;
        return true;
    }

    /*void NodeTemplate::validateConfiguration(const NodePath& path, INodeValidator& validator) const
    {
        // check scale
        if (base::IsNearZero(m_relativeScale.x) || base::IsNearZero(m_relativeScale.y) || base::IsNearZero(m_relativeScale.z))
        {
            validator.reportError(path, "Node scale is zero at one of the components, this will zero the transform determinant, such nodes cannot be instantiated.");
        }
        else if (!IsScaleReasonable(m_relativeScale))
        {
            validator.reportPerfWarning(path, "Node scale settings are unreasonable, math precision issues and/or performance problems may arise.");
        }

        //
    }*/

    //--

    NodeTemplatePtr NodeTemplate::Merge(const NodeTemplate** templatesToMerge, uint32_t numTemplates)
    {
        // no data on input
        if (!templatesToMerge || !numTemplates)
            return nullptr;

        // get the "final" instance template - usually the node that instances the prefab in the world
        auto finalTemplate  = templatesToMerge[numTemplates - 1];

        // setup basic properties
        auto ret = base::CreateSharedPtr<NodeTemplate>();
        ret->m_name = finalTemplate->m_name; // we only care about our final naming and placement
        ret->m_placement = finalTemplate->m_placement;  // TODO: add placement override

        // establish the streaming model
        ret->m_streamingDistanceOverride = 0;
        ret->m_streamingModel = StreamingModel::Auto;
        uint32_t maxComponents = 0;
        for (uint32_t i = 0; i < numTemplates; ++i)
        {
            auto t  = templatesToMerge[i];
            if (t->m_streamingDistanceOverride > 0)
                ret->m_streamingDistanceOverride = t->m_streamingDistanceOverride;

            if (t->m_streamingModel != StreamingModel::Auto)
                if (ret->m_streamingModel != StreamingModel::StreamWithParent && ret->m_streamingModel != StreamingModel::Discard)
                    ret->m_streamingModel = t->m_streamingModel;

            maxComponents += t->m_componentTemplates.size();
        }

        // collect entity templates to merge
        base::InplaceArray<const EntityDataTemplate*, 32> entityTemplatesToMerge;
        for (uint32_t i = 0; i < numTemplates; ++i)
        {
            auto t  = templatesToMerge[i];
            if (t->m_entityTemplate && t->m_entityTemplate->isEnabled())
                entityTemplatesToMerge.pushBack(t->m_entityTemplate.get());
        }

        // create the entity template data
        if (ret->m_entityTemplate = EntityDataTemplate::Merge(entityTemplatesToMerge.typedData(), entityTemplatesToMerge.size()))
            ret->m_entityTemplate->parent(ret);

        // collect all entries for components to merge
        base::HashMap<base::StringID, base::Array<const ComponentDataTemplate*>> componentTemplates;
        componentTemplates.reserve(maxComponents);
        
        for (uint32_t i = 0; i < numTemplates; ++i)
        {
            auto t  = templatesToMerge[i];
            for (auto& comp : t->m_componentTemplates)
            {
                if (comp->isEnabled() && comp->name())
                    componentTemplates[comp->name()].pushBack(comp.get());
            }
        }

        for (auto& templateList : componentTemplates.values())
        {
            if (auto comp = ComponentDataTemplate::Merge(templateList.typedData(), templateList.size()))
            {
                comp->parent(ret);
                ret->m_componentTemplates.pushBack(comp);
            }
        }

        return ret;
    }

    //--

    struct NodeNavigator
    {
        struct Entry
        {
            const NodeTemplateContainer* m_container = nullptr;
            int m_nodeIndex = INDEX_NONE;
        };

        base::InplaceArray<Entry, 10> m_entries;

        NodeNavigator()
        {}

        NodeNavigator(const NodeNavigator& other)
        {
            for (auto& entry : other.m_entries)
                m_entries.emplaceBack(entry);
        }

        void clear()
        {
            m_entries.clear();
        }

        void pushBack(const NodeTemplateContainer* ptr, int nodeIndex)
        {
            auto& entry = m_entries.emplaceBack();
            entry.m_container = ptr;
            entry.m_nodeIndex = nodeIndex;
        }

        void pushFront(const NodeTemplateContainer* ptr, int nodeIndex)
        {
            m_entries.insert(0, { ptr, nodeIndex });
        }

        static int FindChildNodeIndex(const NodeTemplateContainer* container, int nodeIndex, base::StringID childNodeName)
        {
            auto& nodeInfo = container->nodes()[nodeIndex];
            for (auto childNodeIndex : nodeInfo.m_children)
            {
                auto& childNodeInfo = container->nodes()[childNodeIndex];
                if (childNodeInfo.m_data && childNodeInfo.m_data->name() == childNodeName)
                    return childNodeIndex;
            }

            return INDEX_NONE;
        }

        void collectNodeNames(base::HashSet<base::StringID>& outNodeNames) const
        {
            for (auto& entry : m_entries)
            {
                auto& nodeInfo = entry.m_container->nodes()[entry.m_nodeIndex];
                for (auto childNodeIndex : nodeInfo.m_children)
                {
                    auto& childNodeInfo = entry.m_container->nodes()[childNodeIndex];
                    if (childNodeInfo.m_data)
                        outNodeNames.insert(childNodeInfo.m_data->name());
                }
            }
        }

        base::HashSet<base::StringID> validNodeNames() const
        {
            base::HashSet<base::StringID> ret;
            collectNodeNames(ret);
            return std::move(ret);
        }

        void enterChild(base::StringID childNodeName, NodeNavigator& outIt) const
        {
            for (auto& entry : m_entries)
            {
                auto childNodeIndex = FindChildNodeIndex(entry.m_container, entry.m_nodeIndex, childNodeName);
                if (childNodeIndex != INDEX_NONE)
                {
                    auto& childEntry = outIt.m_entries.emplaceBack();
                    childEntry.m_container = entry.m_container;
                    childEntry.m_nodeIndex = childNodeIndex;
                }
            }
        }

        void collectPrefabs(base::Array<PrefabRef>& outPrefabs) const
        {
            for (int i = m_entries.lastValidIndex(); i >= 0; --i)
            {
                auto& entry = m_entries[i];
                auto& nodeInfo = entry.m_container->nodes()[entry.m_nodeIndex];

                for (auto& prefab : nodeInfo.m_data->prefabAssets())
                {
                    if (prefab.m_enabled)
                    {
                        outPrefabs.remove(prefab.m_prefab);
                        outPrefabs.pushBack(prefab.m_prefab); // put in the back
                    }
                }
            }
        }

        NodeTemplatePtr compileNode() const
        {
            base::InplaceArray<const NodeTemplate*, 16> templates;

            for (int i = m_entries.lastValidIndex(); i >= 0; --i)
            {
                auto& entry = m_entries[i];
                auto& nodeInfo = entry.m_container->nodes()[entry.m_nodeIndex];
                if (nodeInfo.m_data)
                    templates.pushBack(nodeInfo.m_data.get());
            }

            return NodeTemplate::Merge(templates.typedData(), templates.size());

        }
    };

    class NodeCompiler
    {
    public:
        NodeCompiler(PrefabDependencies* outDependencies, const NodeTemplateContainerPtr& outputContainer)
            : m_dependencies(outDependencies)
            , m_output(outputContainer)
        {}

        void addDependency(const PrefabRef& prefab, uint32_t dataVersion)
        {
            if (m_dependencies)
            {
                for (auto& dep : m_dependencies->m_entries)
                {
                    if (dep.m_sourcePrefab == prefab.peak())
                    {
                        dep.m_dataVersion = std::min(dep.m_dataVersion, dataVersion);
                        return;
                    }
                }

                auto& newEntry = m_dependencies->m_entries.emplaceBack();
                newEntry.m_sourcePrefab = prefab.peak();
                newEntry.m_dataVersion = dataVersion;
            }
        }

        NodeTemplateContainerPtr prefabContent(const PrefabRef& prefab)
        {
            if (!prefab.peak())
                return nullptr;

            uint32_t dataVersion;
            const auto& content = prefab->nodeContainer(&dataVersion);
            if (!content || content->rootNodes().empty())
                return nullptr;

            addDependency(prefab, dataVersion);

            return content;
        }

        void process(const NodeNavigator& it, const base::AbsoluteTransform& placement, int outputParentNodeIndex)
        {
            base::InplaceArray<PrefabRef, 10> prefabsToInstance;
            it.collectPrefabs(prefabsToInstance);

            NodeNavigator localIt(it);
            for (auto& prefab : prefabsToInstance)
            {
                if (auto content = prefabContent(prefab))
                {
                    auto rootNodeIndex = content->rootNodes()[0];
                    localIt.pushBack(content.get(), rootNodeIndex);
                }
            }

            if (auto node = localIt.compileNode())
            {
                node->placement(placement);

                auto nodeIndex = m_output->addNode(node, false, outputParentNodeIndex);
                if (nodeIndex != INDEX_NONE)
                {
                    for (auto& name : localIt.validNodeNames())
                    {
                        NodeNavigator childIt;
                        localIt.enterChild(name, childIt);
                        process(childIt, placement, nodeIndex);
                    }
                }
            }
        }

    private:
        PrefabDependencies* m_dependencies;
        NodeTemplateContainerPtr m_output;
    };

    void NodeTemplateContainer::compile(int nodeId, const base::AbsoluteTransform& placement, const NodeTemplateContainerPtr& outContainer, int outContainerParentIndex, PrefabDependencies* outDependencies /*= nullptr*/) const
    {
        NodeCompiler compiler(outDependencies, outContainer);
        NodeNavigator it;
        it.pushBack(this, nodeId);
        compiler.process(it, placement, -1);
    }

} // scene
