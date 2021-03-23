/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
***/

#pragma once

#include "core/resource/include/resource.h"
#include "core/containers/include/hashSet.h"
#include "core/resource/include/directTemplate.h"

BEGIN_BOOMER_NAMESPACE()

//--

// a data for prefab in a node
struct ENGINE_WORLD_API RawEntityPrefabSetup
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(RawEntityPrefabSetup);

    bool enabled = true;
    PrefabRef prefab;
    StringID appearance;

    RawEntityPrefabSetup();
    RawEntityPrefabSetup(const PrefabRef& prefab, bool enabled = true);
};

//--

// a behavior data for the entity
struct ENGINE_WORLD_API RawEntityBehaviorEntry
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(RawEntityBehaviorEntry);

    StringID name;
    ObjectIndirectTemplatePtr data;

    RawEntityBehaviorEntry();
};
   
//--

/// a basic template of a node, runtime nodes are spawned from templates
/// node templates form a tree that is transformed once the nodes are instantiated
class ENGINE_WORLD_API RawEntity : public IObject
{
    RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
    RTTI_DECLARE_VIRTUAL_CLASS(RawEntity, IObject);

public:
    RawEntity();

    //--

    // name of the node, can't be empty
    StringID m_name; 

    // list of prefabs to instance AT THIS NODE (some may be disabled)
    Array<RawEntityPrefabSetup> m_prefabAssets;

    // entity data - can be empty if node did not carry any data
    ObjectIndirectTemplatePtr m_entityTemplate;

    // behavior data - can be empty if node does not have any behaviors
    Array<RawEntityBehaviorEntry> m_behaviorTemplates;

    // child nodes
    Array<RawEntityPtr> m_children;

    //--


protected:
    virtual void onPostLoad() override;
};

//--

/// "compiled" node template
struct ENGINE_WORLD_API RawEntityCompiledData : public IReferencable
{
    RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)

public:
    StringID name; // assigned name of the node (NOTE: may be different than name of the node in the prefab)
    Array<RawEntityPtr> templates; // all collected templates from all prefabs that matched the node's path, NOTE: NOT OWNED and NOT modified

    Transform localToParent; // placement of the actual node with respect to parent node, 99% of time matches the one source data
    Transform localToReference; // concatenated transform using the "scale-safe" transform rules, used during instantiation

    RefWeakPtr<RawEntityCompiledData> parent;
    Array<RefPtr<RawEntityCompiledData>> children; // collected children of this node

    //--

    RawEntityCompiledData();

    //--

    // compile an entity from the source data
    // NOTE: this function may load content

    //--
};

//--

// compile flattened node (resolve and embed prefabs referenced in the node, NOTE: only prefabs from root node are embedded, use "Explode" to embed all prefabs)
// NOTE: may return NULL if node contains NO DATA and could be discarded for all practical purposes
ENGINE_WORLD_API RawEntityPtr UnpackTopLevelPrefabs(const RawEntity* rootNode);

// compile flattened node (resolve and embed prefabs referenced in the node, NOTE: only prefabs from root node are embedded, use "Explode" to embed all prefabs)
// NOTE: may return NULL if node contains NO DATA and could be discarded for all practical purposes
ENGINE_WORLD_API RawEntityPtr CompileWithInjectedBaseNodes(const RawEntity* rootNode, const Array<const RawEntity*>& additionalBaseNodes);

// compile an entity from list of templates, may return NULL if no entity data is present in any template
// NOTE: it's not recursive
ENGINE_WORLD_API CAN_YIELD EntityPtr CompileEntity(const Array<const RawEntity*>& templates, bool loadImports, Transform* outEntityLocalTransform = nullptr);

//--

struct ENGINE_WORLD_API HierarchyEntity : public IReferencable
{
    StringID name;
    EntityStaticID id;
    EntityPtr entity;
    Transform localToParent;
    Transform localToWorld;

    bool attachTransformToParentEntity = false;
            
    bool streamingGroupChildren = true;
    bool streamingBreakFromGroup = false;
    float streamingDistanceOverride = 0.0f;

    Array<RefPtr<HierarchyEntity>> children;

    //--

    uint32_t countTotalEntities() const;
    void collectEntities(Array<EntityPtr>& outEntites) const; // TEMPSHIT
};

// compile entity (prefab-style), returns the root entity directly and other all entities via array
ENGINE_WORLD_API CAN_YIELD RefPtr<HierarchyEntity> CompileEntityHierarchy(const EntityStaticIDBuilder& path, const RawEntity* rootTemplateNode, bool loadImports);

//--

END_BOOMER_NAMESPACE()
