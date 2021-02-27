/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
*
***/

#include "build.h"
#include "entity.h"
#include "prefab.h"
#include "path.h"

#include "core/resource/include/objectIndirectTemplate.h"
#include "core/resource/include/objectIndirectTemplateCompiler.h"

BEGIN_BOOMER_NAMESPACE()

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

RTTI_BEGIN_TYPE_CLASS(NodeTemplateBehaviorEntry);
RTTI_PROPERTY(name);
RTTI_PROPERTY(data);
RTTI_END_TYPE();

NodeTemplateBehaviorEntry::NodeTemplateBehaviorEntry()
{}

//--
            
RTTI_BEGIN_TYPE_CLASS(NodeTemplate);
    RTTI_PROPERTY(m_name);
    //RTTI_PROPERTY(m_localToParent);
    RTTI_PROPERTY(m_children);
    RTTI_PROPERTY(m_prefabAssets);
    RTTI_PROPERTY(m_entityTemplate);
    RTTI_PROPERTY(m_behaviorTemplates);
RTTI_END_TYPE();

NodeTemplate::NodeTemplate()
{}

void NodeTemplate::onPostLoad()
{
    TBaseClass::onPostLoad();

    if (!m_entityTemplate)
        m_entityTemplate = RefNew<ObjectIndirectTemplate>();

    for (auto index : m_behaviorTemplates.indexRange().reversed())
        if (!m_behaviorTemplates[index].data)
            m_behaviorTemplates.eraseUnordered(index);

    m_children.removeAll(nullptr);
}

//--

NodeTemplateCompiledData::NodeTemplateCompiledData()
{}

//--

/*typedef HashMap<StringID, ObjectIndirectTemplateCompiler*> ComponentTemplateList;

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
}*/

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

/*ComponentPtr CreateComponentObject(const ObjectIndirectTemplateCompiler& compiler)
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
}*/

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

    /*// gather list of all components to create
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
    }*/

    // TODO: create the initial links between components

    // TODO: place for final fix up of links between entity and components

    // return created entity
    return entity;
}

//--

struct NodeMergeStack
{
    StringID name;
    InplaceArray<const NodeTemplate*, 10> templatesToMerge;
    const NodeTemplate* originalRootNode = nullptr;
};

static void SuckInPrefab(const NodeTemplatePrefabSetup& prefabEntry, HashSet<const Prefab*>& allVisitedPrefabs, Array<const NodeTemplate*>& outPrefabRoots)
{
    if (!prefabEntry.enabled)
        return;

    auto loadedPrefab = prefabEntry.prefab.load();
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
            for (const auto& compInfo : ptr->m_behaviorTemplates)
                if (compInfo.name && compInfo.data)
                    componentData[compInfo.name].pushBack(compInfo.data);

        for (auto pair : componentData.pairs())
        {
            ObjectIndirectTemplateCompiler compiler;
            for (const auto* ptr : pair.value)
                compiler.addTemplate(ptr);

            if (auto behaviorData = compiler.flatten())
            {
                if (behaviorData->templateClass())
                {
                    behaviorData->parent(ret);

                    auto& compEntry = ret->m_behaviorTemplates.emplaceBack();
                    compEntry.name = pair.key;
                    compEntry.data = behaviorData;
                }
            }
        }
    }

    // if we don't have children and local components and carry no data than we don't have to be stored
    if (ret->m_children.empty() && ret->m_behaviorTemplates.empty() && !HasLocalData(ret->m_entityTemplate) && ret->m_prefabAssets.empty())
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
    HashSet<const Prefab*> visitedPrefabs;
    for (const auto& prefab : rootNode->m_prefabAssets) // NOTE: prefab assets are NOT copied to flattened data
        SuckInPrefab(prefab, visitedPrefabs, mergeStack.templatesToMerge);

    // merge the original content with base content
    mergeStack.templatesToMerge.pushBack(rootNode);

    // merge content
    return CompileMergedNode(mergeStack);
}

//--

static const int MAX_DEPTH = 10;

RefPtr<HierarchyEntity> ProcessSingleEntity(int depth, NodePathBuilder& path, const Array<const NodeTemplate*>& inputTemplates, const AbsoluteTransform& placement, bool applyLocalPlacement, res::ResourceLoader* loader)
{
    auto ret = RefNew<HierarchyEntity>();
    ret->id = path.toID();

    InplaceArray<const NodeTemplate*, 20> templates;
    {
        HashSet<const Prefab*> visitedPrefabs;
        for (const auto* temp : inputTemplates)
        {
            for (const auto& prefab : temp->m_prefabAssets) // NOTE: prefab assets are NOT copied to flattened data
                SuckInPrefab(prefab, visitedPrefabs, templates);

            templates.pushBack(temp);
        }
    }

    // compile entity data
    {
        ObjectIndirectTemplateCompiler compiler(loader);
        for (const auto* ptr : templates)
            if (ptr->m_entityTemplate)
                compiler.addTemplate(ptr->m_entityTemplate);
        ret->entity = CreateEntityObject(compiler);

        // place the entity
        if (applyLocalPlacement)
            ret->entity->requestTransform(placement * compiler.compileTransform().toTransform());
        else
            ret->entity->requestTransform(placement);

        // extract streaming information
        ret->streamingGroupChildren = compiler.compileValueOrDefault<bool>("streamingGroupChildren"_id, true);
        ret->streamingBreakFromGroup = compiler.compileValueOrDefault<bool>("streamingBreakFromGroup"_id, false);
        ret->streamingDistanceOverride = compiler.compileValueOrDefault<float>("streamingDistanceOverride"_id, 0.0f);
    }

    //--

    // collect and create components
    /*{
        HashMap<StringID, Array<const ObjectIndirectTemplate*>> componentData;
        for (const auto* ptr : templates)
            for (const auto& compInfo : ptr->m_componentTemplates)
                if (compInfo.name && compInfo.data)
                    componentData[compInfo.name].pushBack(compInfo.data);

        for (auto pair : componentData.pairs())
        {
            ObjectIndirectTemplateCompiler compiler(loader);
            for (const auto* ptr : pair.value)
                compiler.addTemplate(ptr);

            if (auto comp = CreateComponentObject(compiler))
            {
                const auto relativePlacement = compiler.compileTransform();
                comp->relativeTransform(relativePlacement.toTransform());
                ret->entity->attachComponent(comp);
            }                   

        }
    }*/

    // collect and create child entities
    if (depth < MAX_DEPTH)
    {
        HashMap<StringID, Array<const NodeTemplate*>> childrenNodes;
        for (const auto* ptr : templates)
            for (const auto& child : ptr->m_children)
                if (child->m_name)
                    childrenNodes[child->m_name].pushBack(child);

        for (auto pair : childrenNodes.pairs())
        {
            path.pushSingle(pair.key);

            if (auto child = ProcessSingleEntity(depth + 1, path, pair.value, ret->entity->absoluteTransform(), true, loader))
                ret->children.pushBack(child);

            path.pop();
        }
    }

    return ret;
}

void HierarchyEntity::collectEntities(Array<EntityPtr>& outEntites) const
{
    outEntites.pushBack(entity);

    for (const auto& child : children)
        child->collectEntities(outEntites);
}

uint32_t HierarchyEntity::countTotalEntities() const
{
    uint32_t ret = 1;

    for (const auto& child : children)
        ret += child->countTotalEntities();

    return ret;
}

RefPtr<HierarchyEntity> CompileEntityHierarchy(const NodePathBuilder& path, const NodeTemplate* rootTemplateNode, const AbsoluteTransform* forceInitialPlacement, res::ResourceLoader* loader)
{
    InplaceArray<const NodeTemplate*, 10> nodeTemplates;
    if (rootTemplateNode)
        nodeTemplates.pushBack(rootTemplateNode);

    NodePathBuilder localPath(path);
    if (forceInitialPlacement)
        return ProcessSingleEntity(0, localPath, nodeTemplates, *forceInitialPlacement, false, loader);
    else
        return ProcessSingleEntity(0, localPath, nodeTemplates, AbsoluteTransform::ROOT(), true, loader);
}

//--

END_BOOMER_NAMESPACE()
