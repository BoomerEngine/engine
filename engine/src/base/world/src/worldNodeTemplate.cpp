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

#include "base/object/include/object.h"
#include "worldNodePath.h"

namespace base
{
    namespace world
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

        RTTI_BEGIN_TYPE_CLASS(NodeTemplateComponentEntry);
        RTTI_PROPERTY(name);
        RTTI_PROPERTY(data);
        RTTI_END_TYPE();

        NodeTemplateComponentEntry::NodeTemplateComponentEntry()
        {}

        //--

        RTTI_BEGIN_TYPE_CLASS(NodeTemplate);
            RTTI_PROPERTY(m_name);
            RTTI_PROPERTY(m_children);
            RTTI_PROPERTY(m_prefabAssets);
            RTTI_PROPERTY(m_entityTemplate);
            RTTI_PROPERTY(m_componentTemplates);
        RTTI_END_TYPE();

        NodeTemplate::NodeTemplate()
        {}

        const ComponentTemplate* NodeTemplate::findComponent(StringID name) const
        {
            for (const auto& data : m_componentTemplates)
                if (data.name == name)
                    return data.data;

            return nullptr;
        }

        const NodeTemplate* NodeTemplate::findChild(StringID name) const
        {
            for (const auto& data : m_children)
                if (data->m_name == name)
                    return data;

            return nullptr;
        }

        //--

        NodeCompilationStack::NodeCompilationStack()
        {}

        NodeCompilationStack::NodeCompilationStack(const NodeCompilationStack& other)
        {
            for (const auto* data : other.m_templates)
                m_templates.pushBack(data);
        }


        void NodeCompilationStack::clear()
        {
            m_templates.reset();
        }

        void NodeCompilationStack::pushBack(const NodeTemplate* ptr)
        {
            DEBUG_CHECK_RETURN(ptr != nullptr);
            m_templates.pushBack(ptr);
        }

        void NodeCompilationStack::collectChildNodeNames(HashSet<StringID>& outNodeNames) const
        {
            for (const auto& entry : m_templates)
                for (const auto& child : entry->m_children)
                    if (child && !child->m_name.empty())
                        outNodeNames.insert(child->m_name);
        }

        void NodeCompilationStack::collectPrefabs(Array<PrefabRef>& outPrefabs) const
        {
            for (auto i : m_templates.indexRange().reversed())
            {
                const auto& nodeInfo = m_templates[i];
                for (const auto& prefab : nodeInfo->m_prefabAssets)
                {
                    if (prefab.enabled && prefab.prefab)
                    {
                        outPrefabs.remove(prefab.prefab);
                        outPrefabs.pushBack(prefab.prefab); // if a prefab was re-added then put in the back (overlay)
                    }
                }
            }
        }

        void NodeCompilationStack::enterChild(StringID childNodeName, NodeCompilationStack& outIt) const
        {
            outIt.m_templates.reset();

            for (const auto& entry : m_templates)
                if (const auto* childTemplate = entry->findChild(childNodeName))
                    outIt.m_templates.pushBack(childTemplate);
        }

        //--

        template< typename T >
        static Transform MergeTransform(const Array<const T*>& templates)
        {
            Transform ret;
            bool first = true;

            for (auto i : templates.indexRange().reversed())
            {
                const auto* data = templates[i];
                DEBUG_CHECK(data != nullptr);
                if (!data->placement().isIdentity())
                {
                    if (first)
                        ret = data->placement().toTransform();
                    else
                        ret = data->placement().toTransform().applyTo(ret);
                }
            }

            return ret;
        }

        template< typename T >
        static RefPtr<T> MergeTemplates(const Array<const T*>& templates)
        {
            if (templates.size() == 0)
                return nullptr;

            if (templates.size() == 1)
                return AddRef(templates[0]);

            RefPtr<T> ret = nullptr;
            for (auto i : templates.indexRange().reversed())
            {
                auto copy = CloneObject(templates[i]);
                copy->rebase(ret);
                ret = copy;
            }

            return ret;
        }

        typedef HashMap<StringID, Array<const ComponentTemplate*>>ComponentTemplateList;

        static void CollectComponentTemplates(const Array<const NodeTemplate*>& templates, ComponentTemplateList& outTemplates)
        {
            for (const auto* dataTemplate : templates)
                for (const auto& compTemplate : dataTemplate->m_componentTemplates)
                    if (compTemplate.name && compTemplate.data && compTemplate.data->enabled())
                        outTemplates[compTemplate.name].pushBack(compTemplate.data);
        }

        EntityPtr CompileEntity(const Array<const NodeTemplate*>& templates, Transform* outEntityLocalTransform /*= nullptr*/)
        {
            PC_SCOPE_LVL1(CreateEntity);

            // collect valid entity templates to build an entity from
            InplaceArray<const EntityTemplate*, 8> entityTemplates;
            for (const auto& dataTemplate : templates)
            {
                const auto& entityData = dataTemplate->m_entityTemplate;
                if (entityData && entityData->enabled())
                    entityTemplates.pushBack(entityData);
            }

            // merge the entity template
            const auto mergedEntityTemplate = MergeTemplates(entityTemplates);
            if (!mergedEntityTemplate)
                return nullptr;

            // compute the merged transform
            if (outEntityLocalTransform)
                *outEntityLocalTransform = MergeTransform(entityTemplates);

            // figure out the entity class, use the default entity class if nothing was specified
            // TODO: use "static entity" in that case so we can PURGE more stuff in level cooking
            auto entity = mergedEntityTemplate->createEntity();

            // always create a default entity if the template fails here
            if (!entity)
                entity = CreateSharedPtr<Entity>();

            // gather list of all components to create
            ComponentTemplateList namedComponentTemplates;
            CollectComponentTemplates(templates, namedComponentTemplates);

            // create all named components and attach them to entity
            namedComponentTemplates.forEach([&entity](StringID name, const Array<const ComponentTemplate*>& templates)
                {
                    if (const auto componentTemplate = MergeTemplates(templates))
                    {
                        if (auto component = componentTemplate->createComponent())
                        {
                            const auto componentTransform = MergeTransform(templates);
                            component->bindName(name);
                            component->relativeTransform(componentTransform);
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

        static const int MAX_DEPTH = 10;

        EntityPtr ProcessSingleEntity(int depth, const NodeCompilationStack& it, const AbsoluteTransform& placement, const NodePath& path, Array<EntityPtr>& outAllEntities)
        {
            InplaceArray<PrefabRef, 10> prefabsToInstance;
            it.collectPrefabs(prefabsToInstance);

            // "instance" prefabs
            NodeCompilationStack localIt(it);
            for (auto& prefab : prefabsToInstance)
            {
                if (auto data = prefab.acquire())
                {
                    const auto rootIndex = 0;
                    if (rootIndex >= 0 && rootIndex <= data->nodes().lastValidIndex())
                    {
                        if (const auto rootNode = data->nodes()[rootIndex])
                            localIt.pushBack(rootNode);
                    }
                }
            }

            // compile single entity out of the current stack
            Transform relativePlacementTransform;
            if (auto compiledNode = CompileEntity(localIt.templates(), &relativePlacementTransform))
            {
                outAllEntities.pushBack(compiledNode);

                // apply entity transform, NOTE: the root entity is not moved
                const auto entityPlacement = depth ? (placement * relativePlacementTransform) : placement;
                compiledNode->requestTransform(entityPlacement);

                // create child entities
                if (depth < MAX_DEPTH)
                {
                    HashSet<StringID> childrenNames;
                    localIt.collectChildNodeNames(childrenNames);
                    for (const auto name : childrenNames)
                    {
                        NodeCompilationStack childIt;
                        localIt.enterChild(name, childIt);

                        const auto childPath = path[name];
                        ProcessSingleEntity(depth + 1, childIt, entityPlacement, childPath, outAllEntities);
                    }
                }

                return compiledNode;
            }
            else
            {
                return nullptr;
            }
        }

        EntityPtr CompileEntityHierarchy(const NodeTemplate* rootTemplateNode, const AbsoluteTransform& placement, const NodePath& path, Array<EntityPtr>& outAllEntities)
        {
            NodeCompilationStack stack;
            stack.pushBack(rootTemplateNode);

            return ProcessSingleEntity(0, stack, placement, path, outAllEntities);
        }

        //--

    } // world
} // game