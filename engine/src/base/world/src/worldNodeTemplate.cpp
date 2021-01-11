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
#include "worldNodePath.h"

#include "base/resource/include/objectIndirectTemplate.h"
#include "base/resource/include/objectIndirectTemplateCompiler.h"

namespace base
{
    namespace world
    {

        //--

        RTTI_BEGIN_TYPE_STRUCT(NodeTemplatePrefabSetup);
        RTTI_PROPERTY(enabled);
        RTTI_PROPERTY(prefab);
        RTTI_PROPERTY(appearance);
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
            //RTTI_PROPERTY(m_localToParent);
            RTTI_PROPERTY(m_children);
            RTTI_PROPERTY(m_prefabAssets);
            RTTI_PROPERTY(m_entityTemplate);
            RTTI_PROPERTY(m_componentTemplates);
        RTTI_END_TYPE();

        NodeTemplate::NodeTemplate()
        {}

        void NodeTemplate::onPostLoad()
        {
            TBaseClass::onPostLoad();

            if (!m_entityTemplate)
                m_entityTemplate = RefNew<ObjectIndirectTemplate>();

            for (auto index : m_componentTemplates.indexRange().reversed())
                if (!m_componentTemplates[index].data)
                    m_componentTemplates.eraseUnordered(index);
        }

        const ObjectIndirectTemplate* NodeTemplate::findComponent(StringID name) const
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

        NodeTemplateCompiledData::NodeTemplateCompiledData()
        {}

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

        typedef HashMap<StringID, ObjectIndirectTemplateCompiler*> ComponentTemplateList;

        static void CollectComponentTemplates(const Array<const NodeTemplate*>& templates, ComponentTemplateList& outTemplates)
        {
            for (const auto* dataTemplate : templates)
            {
                for (const auto& compTemplate : dataTemplate->m_componentTemplates)
                {
                    if (compTemplate.name && compTemplate.data && compTemplate.data->enabled())
                    {
                        auto*& compilerPtr = outTemplates[compTemplate.name];
                        if (!compilerPtr)
                            compilerPtr = new ObjectIndirectTemplateCompiler();
                        compilerPtr->addTemplate(compTemplate.data);
                    }
                }
            }
        }

        EntityPtr CreateEntityObject(const ObjectIndirectTemplateCompiler& compiler)
        {
            auto entityClass = compiler.compileClass().cast<Entity>();
            if (entityClass)
            {
                auto defaultEntity = (Entity*)entityClass->defaultObject();
                entityClass = defaultEntity->determineEntityTemplateClass(compiler);
                if (!entityClass)
                    return nullptr;
            }

            if (!entityClass)
                entityClass = Entity::GetStaticClass();

            auto entity = entityClass->create<Entity>();
            if (!entity->initializeFromTemplateProperties(compiler))
                return nullptr;

            return entity;
        }

        ComponentPtr CreateComponentObject(const ObjectIndirectTemplateCompiler& compiler)
        {
            auto componentClass = compiler.compileClass().cast<Component>();
            if (!componentClass)
                return nullptr;

            auto defaultComponent = (Component*)componentClass->defaultObject();
            componentClass = defaultComponent->determineComponentTemplateClass(compiler);
            if (!componentClass)
                return nullptr;

            auto component = componentClass->create<Component>();
            if (!component->initializeFromTemplateProperties(compiler))
                return nullptr;

            return component;
        }

        EntityPtr CompileEntity(const Array<const NodeTemplate*>& templates, Transform* outEntityLocalTransform /*= nullptr*/)
        {
            PC_SCOPE_LVL1(CreateEntity);

            // collect valid entity templates to build an entity from
            ObjectIndirectTemplateCompiler entityTemplateCompiler;
            for (const auto& dataTemplate : templates)
            {
                const auto& entityData = dataTemplate->m_entityTemplate;
                if (entityData && entityData->enabled())
                    entityTemplateCompiler.addTemplate(entityData);
            }

            // compute the merged transform
            if (outEntityLocalTransform)
                *outEntityLocalTransform = entityTemplateCompiler.compileTransform().toTransform();

            // figure out the entity class, use the default entity class if nothing was specified
            // TODO: use "static entity" in that case so we can PURGE more stuff in level cooking
            auto entity = CreateEntityObject(entityTemplateCompiler);

            // always create a default entity if the template fails here
            if (!entity)
                entity = RefNew<Entity>();

            // gather list of all components to create
            ComponentTemplateList namedComponentTemplates;
            CollectComponentTemplates(templates, namedComponentTemplates);

            // create all named components and attach them to entity
            for (auto pair : namedComponentTemplates.pairs())
            {
                if (auto component = CreateComponentObject(*pair.value))
                {
                    component->bindName(pair.key);
                    component->relativeTransform(pair.value->compileTransform().toTransform());
                    entity->attachComponent(component);
                }
            }

            namedComponentTemplates.clearPtr();

            // TODO: create the initial links between components

            // TODO: place for final fix up of links between entity and components

            // return created entity
            return entity;
        }

        //--

        static const int MAX_DEPTH = 10;

        EntityPtr ProcessSingleEntity(int depth, const NodeCompilationStack& it, const AbsoluteTransform& placement, const NodePath& path, Array<EntityPtr>& outAllEntities)
        {
            /*InplaceArray<PrefabRef, 10> prefabsToInstance;
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
            }*/
            return nullptr;
        }

        EntityPtr CompileEntityHierarchy(const NodeTemplate* rootTemplateNode, const AbsoluteTransform& placement, const NodePath& path, Array<EntityPtr>& outAllEntities)
        {
            NodeCompilationStack stack;
            stack.pushBack(rootTemplateNode);

            return ProcessSingleEntity(0, stack, placement, path, outAllEntities);
        }

        //--

        struct NodeMergeStack
        {
            StringID name;
            InplaceArray<const NodeTemplate*, 10> templatesToMerge;
            const NodeTemplate* originalRootNode = nullptr;
        };

        static void SuckInPrefab(const world::NodeTemplatePrefabSetup& prefabEntry, HashSet<const world::Prefab*>& allVisitedPrefabs, Array<const world::NodeTemplate*>& outPrefabRoots)
        {
            if (!prefabEntry.enabled)
                return;

            auto loadedPrefab = prefabEntry.prefab.acquire();
            if (!loadedPrefab)
                return;

            if (!allVisitedPrefabs.insert(loadedPrefab))
                return;

            auto prefabRootNode = loadedPrefab->root();
            if (!prefabRootNode)
                return;

            for (const auto& rootPrefab : prefabRootNode->m_prefabAssets)
                SuckInPrefab(rootPrefab, allVisitedPrefabs, outPrefabRoots);

            outPrefabRoots.pushBack(prefabRootNode);
        }

        static bool HasLocalData(const ObjectIndirectTemplate* data)
        {
            if (!data)
                return false;

            if (!data->templateClass().empty())
                return true;

            if (!data->properties().empty())
                return true;

            if (!data->placement().isIdentity())
                return true;

            return false;
        }

        static NodeTemplatePtr CompileMergedNode(const NodeMergeStack& stack)
        {
            auto ret = RefNew<NodeTemplate>();
            ret->m_name = stack.name;

            // compile entity data
            {
                ObjectIndirectTemplateCompiler compiler;
                for (const auto* ptr : stack.templatesToMerge)
                    if (ptr->m_entityTemplate)
                        compiler.addTemplate(ptr->m_entityTemplate);
                ret->m_entityTemplate = compiler.flatten();
                ret->m_entityTemplate->parent(ret);
            }

            // merge template list
            for (const auto* ptr : stack.templatesToMerge)
                if (ptr != stack.originalRootNode) // NOTE: skip original root node as the prefabs expanded for it
                    for (const auto& prefabInfo : ptr->m_prefabAssets)
                        ret->m_prefabAssets.pushBack(prefabInfo);

            // collect and create children
            {
                HashMap<StringID, Array<const NodeTemplate*>> childrenNodes;                
                for (const auto* ptr : stack.templatesToMerge)
                    for (const auto& child : ptr->m_children)
                        if (child->m_name)
                            childrenNodes[child->m_name].pushBack(child);

                for (auto pair : childrenNodes.pairs())
                {
                    NodeMergeStack childMergeStack;
                    childMergeStack.name = pair.key;
                    childMergeStack.templatesToMerge = std::move(pair.value);

                    if (auto mergedChild = CompileMergedNode(childMergeStack)) // NOTE: we may still get an empty node (ie. no overrides)
                    {
                        mergedChild->parent(ret);
                        ret->m_children.pushBack(mergedChild);
                    }
                }
            }

            // collect and create components
            {
                HashMap<StringID, Array<const ObjectIndirectTemplate*>> componentData;
                for (const auto* ptr : stack.templatesToMerge)
                    for (const auto& compInfo : ptr->m_componentTemplates)
                        if (compInfo.name && compInfo.data)
                            componentData[compInfo.name].pushBack(compInfo.data);

                for (auto pair : componentData.pairs())
                {
                    ObjectIndirectTemplateCompiler compiler;
                    for (const auto* ptr : pair.value)
                        compiler.addTemplate(ptr);

                    if (auto componentData = compiler.flatten())
                    {
                        if (HasLocalData(componentData))
                        {
                            componentData->parent(ret);

                            auto& compEntry = ret->m_componentTemplates.emplaceBack();
                            compEntry.name = pair.key;
                            compEntry.data = componentData;
                        }
                    }
                }
            }

            // if we don't have children and local components and carry no data than we don't have to be stored
            if (ret->m_children.empty() && ret->m_componentTemplates.empty() && !HasLocalData(ret->m_entityTemplate) && ret->m_prefabAssets.empty())
                return nullptr;

            // valid node
            return ret;
        }

        NodeTemplatePtr CompileWithInjectedBaseNodes(const NodeTemplate* rootNode, const Array<const NodeTemplate*>& additionalBaseNodes)
        {
            // always merge the original editable node - if there are no prefabs we will get a copy of data
            NodeMergeStack mergeStack;

            // merge the original content with base content
            for (const auto* baseNode : additionalBaseNodes)
                if (baseNode)
                    mergeStack.templatesToMerge.pushBack(baseNode);

            mergeStack.templatesToMerge.pushBack(rootNode);

            // merge content
            return CompileMergedNode(mergeStack);
        }

        NodeTemplatePtr UnpackTopLevelPrefabs(const NodeTemplate* rootNode)
        {
            // always merge the original editable node - if there are no prefabs we will get a copy of data
            NodeMergeStack mergeStack;
            mergeStack.originalRootNode = rootNode;

            // collect base nodes from prefabs used in the node we want to flatten
            HashSet<const world::Prefab*> visitedPrefabs;
            for (const auto& prefab : rootNode->m_prefabAssets) // NOTE: prefab assets are NOT copied to flattened data
                SuckInPrefab(prefab, visitedPrefabs, mergeStack.templatesToMerge);

            // merge the original content with base content
            mergeStack.templatesToMerge.pushBack(rootNode);

            // merge content
            return CompileMergedNode(mergeStack);
        }

        //--

    } // world
} // base