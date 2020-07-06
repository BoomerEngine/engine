/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
*
***/

#include "build.h"
#include "worldEntity.h"
#include "worldComponent.h"
#include "worldPrefab.h"
#include "worldEntityTemplate.h"
#include "worldComponentTemplate.h"
#include "worldNodePlacement.h"
#include "worldNodeContainer.h"

#include "base/object/include/object.h"
#include "worldNodePath.h"

namespace game
{

    //--

    RTTI_BEGIN_TYPE_STRUCT(NodeTemplatePrefabSetup);
        RTTI_PROPERTY(enabled);
        RTTI_PROPERTY(prefab);
    RTTI_END_TYPE();

    NodeTemplatePrefabSetup::NodeTemplatePrefabSetup()
    {}

    NodeTemplatePrefabSetup::NodeTemplatePrefabSetup(const PrefabRef& prefab_, bool enabled_ /*= true*/)
        : prefab(prefab_)
        , enabled(enabled_)
    {}

    NodeTemplatePrefabSetup::NodeTemplatePrefabSetup(const PrefabPtr& prefab_, bool enabled_ /*= true*/)
        : prefab(prefab_)
        , enabled(enabled_)
    {}

    //--

    RTTI_BEGIN_TYPE_ENUM(NodeTemplateType);
        RTTI_ENUM_OPTION(Content);
        RTTI_ENUM_OPTION(Override);
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

    //--

    RTTI_BEGIN_TYPE_CLASS(NodeTemplateComponentEntry);
        RTTI_PROPERTY(name);
        RTTI_PROPERTY(data);
    RTTI_END_TYPE();

    NodeTemplateComponentEntry::NodeTemplateComponentEntry()
    {}

    NodeTemplateComponentEntry::NodeTemplateComponentEntry(base::StringID name_, ComponentTemplate* data_)
        : name(name_)
        , data(AddRef(data_))
    {}

    //--

    NodeTemplateConstructionInfo::NodeTemplateConstructionInfo()
    {}

    //--

    RTTI_BEGIN_TYPE_CLASS(NodeTemplate);
        RTTI_CATEGORY("Node");
        RTTI_PROPERTY(m_name).editable("Name of the node");
        RTTI_CATEGORY("Placement");
        RTTI_PROPERTY(m_placement).editable();
        RTTI_CATEGORY("Streaming");
        RTTI_PROPERTY(m_streamingModel).editable("How the node should be streamed in game");
        RTTI_PROPERTY(m_streamingDistanceOverride).editable("Custom maximum visibility distance in meters for the node");

        RTTI_PROPERTY(m_prefabAssets);
        RTTI_PROPERTY(m_entityTemplate);
        RTTI_PROPERTY(m_componentTemplates);
    RTTI_END_TYPE();

    NodeTemplate::NodeTemplate()
    {}

    NodeTemplate::NodeTemplate(const NodeTemplateConstructionInfo& data)
        : m_name(data.name)
        , m_placement(data.placement)
        , m_type(data.type)
        , m_prefabAssets(data.prefabAssets)
        , m_streamingModel(data.streamingModel)
        , m_streamingDistanceOverride(data.streamingDistanceOverride)
        , m_entityTemplate(data.entityData)
        , m_componentTemplates(data.componentData)
    {}

    NodeTemplate::~NodeTemplate()
    {}

    ComponentTemplatePtr NodeTemplate::findComponentTemplate(base::StringID name) const
    {
        for (const auto& data : m_componentTemplates)
            if (data.name == name)
                return data.data;

        return nullptr;
    }

    //--

    struct NodeNavigator
    {
        struct Entry
        {
            const NodeTemplateContainer* container = nullptr;
            int nodeIndex = INDEX_NONE;
            base::StringID name;
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

        void pushBack(const NodeTemplateContainer* ptr, int nodeIndex, base::StringID name = base::StringID::EMPTY())
        {
            auto& entry = m_entries.emplaceBack();
            entry.container = ptr;
            entry.nodeIndex = nodeIndex;
            entry.name = name ? name : ptr->nodes()[nodeIndex].m_data->name();
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
                auto& nodeInfo = entry.container->nodes()[entry.nodeIndex];
                for (auto childNodeIndex : nodeInfo.m_children)
                {
                    auto& childNodeInfo = entry.container->nodes()[childNodeIndex];
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
                auto childNodeIndex = FindChildNodeIndex(entry.container, entry.nodeIndex, childNodeName);
                if (childNodeIndex != INDEX_NONE)
                {
                    auto& childEntry = outIt.m_entries.emplaceBack();
                    childEntry.container = entry.container;
                    childEntry.nodeIndex = childNodeIndex;
                    childEntry.name = childNodeName;
                }
            }
        }

        void collectPrefabs(base::Array<PrefabRef>& outPrefabs) const
        {
            for (int i = m_entries.lastValidIndex(); i >= 0; --i)
            {
                auto& entry = m_entries[i];
                auto& nodeInfo = entry.container->nodes()[entry.nodeIndex];

                for (auto& prefab : nodeInfo.m_data->prefabAssets())
                {
                    if (prefab.enabled && prefab.prefab)
                    {
                        outPrefabs.remove(prefab.prefab);
                        outPrefabs.pushBack(prefab.prefab); // if a prefab was re-added then put in the back (overlay)
                        // TODO: better strategy for readded prefabs 
                    }
                }
            }
        }

        NodeTemplateCompiledDataPtr compileNode() const
        {
            auto ret = base::CreateSharedPtr<NodeTemplateCompiledData>();

            for (int i = m_entries.lastValidIndex(); i >= 0; --i)
            {
                auto& entry = m_entries[i];
                auto& nodeInfo = entry.container->nodes()[entry.nodeIndex];

                ret->name = entry.name;

                if (nodeInfo.m_data)
                    ret->templates.pushBack(nodeInfo.m_data);
            }

            DEBUG_CHECK_EX(!ret->templates.empty(), "No data for a node - how did we know about it then?");
            return ret;
        }
    };

    class NodeCompiler
    {
    public:
        NodeCompiler(NodeTemplateCompiledOutput& outputContainer)
            : m_output(outputContainer)
        {}

        void addDependency(const PrefabRef& prefab)
        {
            m_output.allUsedPrefabs.insert(prefab.acquire());
        }

        NodeTemplateContainerPtr prefabContent(const PrefabRef& prefab)
        {
            auto data = prefab.acquire();
            if (!data)
                return nullptr;

            const auto& content = data->content();
            if (!content || content->rootNodes().empty())
                return nullptr;

            addDependency(prefab);
            return content;
        }

        void process(const NodeNavigator& it, NodeTemplateCompiledData* compiledParent)
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

            if (auto compiledNode = localIt.compileNode())
            {
                DEBUG_CHECK_EX(compiledNode->name, "Node has no name");
                m_output.allNodes.pushBack(compiledNode);

                if (compiledParent == nullptr)
                    m_output.roots.pushBack(compiledNode);
                else
                    compiledParent->children.pushBack(compiledNode);

                for (auto& name : localIt.validNodeNames())
                {
                    NodeNavigator childIt;
                    localIt.enterChild(name, childIt);
                    process(childIt, compiledNode);
                }
            }
        }

    private:
        NodeTemplateCompiledOutput& m_output;
    };

    void NodeTemplateContainer::compileNode(int nodeId, base::StringID nodeName, NodeTemplateCompiledOutput& outContainer, NodeTemplateCompiledData* outContainerParent) const
    {
        NodeCompiler compiler(outContainer);

        NodeNavigator it;
        it.pushBack(this, nodeId, nodeName);

        compiler.process(it, outContainerParent);
    }

    void NodeTemplateContainer::compile(NodeTemplateCompiledOutput& outContainer, NodeTemplateCompiledData* compiledParent) const
    {
        NodeCompiler compiler(outContainer);

        // compile all the roots
        for (const auto rootNodeId : rootNodes())
        {
            NodeNavigator it;
            it.pushBack(this, rootNodeId);
            compiler.process(it, compiledParent);
        }
    }

    //--

    NodeTemplateCompiledData::NodeTemplateCompiledData()
    {}

    EntityPtr NodeTemplateCompiledData::createEntity(const base::AbsoluteTransform& parentTransform) const CAN_YIELD
    {
        PC_SCOPE_LVL1(CreateEntity);

        // collect valid entity templates to build an entity from
        base::InplaceArray<EntityTemplatePtr, 8> entityTemplates;
        base::Transform entityTransform;
        for (const auto& dataTemplate : templates)
        {
            if (dataTemplate->entityTemplate())
            {
                DEBUG_CHECK_EX(dataTemplate->entityTemplate()->m_enabled, "Disabled templates should not be collected");
                if (dataTemplate->entityTemplate()->m_enabled)
                {
                    // only the content template can set the type of the data
                    if (dataTemplate->type() == NodeTemplateType::Content)
                    {
                        entityTemplates.reset(); // every "content" node will reset the list as previous overrides become inapplicable
                        entityTemplates.pushBack(dataTemplate->entityTemplate());
                        entityTransform = dataTemplate->placement().toTransform();
                    }

                    // we can only apply overrides to content nodes
                    else if (!entityTemplates.empty())
                    {
                        entityTemplates.pushBack(dataTemplate->entityTemplate());
                        entityTransform = dataTemplate->placement().toTransform().applyTo(entityTransform);
                    }
                }
            }
        }

        // figure out the entity class, use the default entity class if nothing was specified
        // TODO: use "static entity" in that case so we can PURGE more stuff in level cooking
        auto entity = entityTemplates.empty() ? base::CreateSharedPtr<Entity>() : entityTemplates.front()->createEntity();

        // always create a default entity
        if (!entity)
            entity = base::CreateSharedPtr<Entity>();

        // create the entity
        entity->requestTransform(parentTransform * entityTransform);

        // apply entity parameters from the template
        for (uint32_t i=1; i<entityTemplates.size(); ++i)
            entityTemplates[i]->applyOverrides(entity);

        // gather list of all components to create
        // TODO: optimize...
        base::HashMap<base::StringID, base::Array<ComponentTemplatePtr>> namedComponentTemplates;
        for (const auto& dataTemplate : templates)
        {
            for (const auto& compTemplate : dataTemplate->componentTemplates())
            {
                DEBUG_CHECK_EX(compTemplate.name, "Component has no name");
                DEBUG_CHECK_EX(compTemplate.data->m_enabled, "Disabled templates should not be collected");

                if (compTemplate.name && compTemplate.data->m_enabled)
                {
                    if (dataTemplate->type() == NodeTemplateType::Content) // content nodes override the previous content/overrides
                    {
                        namedComponentTemplates[compTemplate.name].reset();
                        namedComponentTemplates[compTemplate.name].pushBack(compTemplate.data);
                    }
                    else if (auto* componentTemplates = namedComponentTemplates.find(compTemplate.name))
                    {
                        // there must be a template already
                        if (!componentTemplates->empty())
                            componentTemplates->pushBack(compTemplate.data);
                    }
                }

            }
        }

        // create all named components
        // TODO: any particular order ?
        base::HashMap<base::StringID, ComponentPtr> createdComponents;
        createdComponents.reserve(namedComponentTemplates.size());
        namedComponentTemplates.forEach([&entity, &createdComponents](base::StringID name, const base::Array<ComponentTemplatePtr>& templates)
            {
                if (!templates.empty())
                {
                    // resolve the class of the component to create
                    // TODO: allow for down shifting the class by overrides ? is MeshComponent -> MySpecialMeshComponent ?
                    auto component = templates.front()->createComponent();
                    if (component)
                    {
                        // store
                        createdComponents[name] = component;

                        // calculate transform
                        base::Transform componentTransaform = templates.front()->m_placement.toTransform();
                        for (uint32_t i = 1; i < templates.size(); ++i)
                            componentTransaform = templates[i]->m_placement.toTransform().applyTo(componentTransaform);
                        component->relativeTransform(componentTransaform);

                        // apply data overrides
                        for (uint32_t i=1; i<templates.size(); ++i)
                            templates[i]->applyOverrides(component);

                        // add to entity
                        entity->attachComponent(component);
                    }
                }
            });       


        // TODO: create the initial links between components

        // TODO: place for final fix up of links between entity and components

        // return created entity
        return entity;        
    }

    //--

    NodeTemplateCompiledOutput::NodeTemplateCompiledOutput()
    {}

    void NodeTemplateCompiledOutput::createSingleRoot(int rootIndex, const NodePath& rootPath, const base::AbsoluteTransform& rootTransform, NodeTemplateCreatedEntities& outEntities) const
    {
        if (rootIndex < 0 || rootIndex > roots.lastValidIndex())
            return;

        if (auto root = createEntity(roots[rootIndex], rootPath, rootTransform, outEntities))
            outEntities.rootEntities.pushBack(root);
    }

    EntityPtr NodeTemplateCompiledOutput::createEntity(const NodeTemplateCompiledData* nodeData, const NodePath& nodePath, const base::AbsoluteTransform& parentTransform, NodeTemplateCreatedEntities& outEntities) const
    {
        auto entity = nodeData->createEntity(parentTransform);
        if (entity)
        {
            outEntities.allEntities.pushBack(entity);

            for (const auto& child : nodeData->children)
            {
                const auto childPath = nodePath[child->name.view()];
                createEntity(child, childPath, entity->absoluteTransform(), outEntities);
            }
        }

        return entity;
    }

    //--

} // game
