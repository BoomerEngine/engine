/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\structure #]
***/

#pragma once

#include "engine/world/include/nodeTemplate.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

/// local node visibility state
enum class SceneContentNodeLocalVisibilityState : uint8_t
{
    Default,
    Visible,
    Hidden,
};

//--

/// general visual flags
enum class SceneContentNodeVisualBit : uint32_t
{
    ActiveNode = FLAG(0),
    SelectedNode = FLAG(1), // directly selected
};

typedef DirectFlags<SceneContentNodeVisualBit> SceneContentNodeVisualFlags;

//--

// general scene node structure
class EDITOR_SCENE_EDITOR_API SceneContentNode : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneContentNode, IObject);

public:
    SceneContentNode(SceneContentNodeType type, const StringBuf& name);

    //--

    // get the node type
    INLINE SceneContentNodeType type() const { return m_type; }

    // get the node name
    INLINE const StringBuf& name() const { return m_name; }

    // get the structure we are part of, can be NULL for floating nodes (ie. clipboard, undo steps, etc)
    INLINE SceneContentStructure* structure() const { return m_structure; }

    // get parent object as if it's a parent node
    INLINE SceneContentNode* parent() const { return rtti_cast<SceneContentNode>(IObject::parent()); }

    // are we attached to scene structure ?
    INLINE bool attached() const { return m_structure != nullptr; }

    // is the node visible ? (true only if all parent nodes are visible as well)
    INLINE bool visible() const { return m_visible; }

    // local visiblity flag - false if user hidden this node specifically
    INLINE SceneContentNodeLocalVisibilityState visibilityFlagRaw() const { return m_localVisibilityFlag; }

    // child nodes
    INLINE const Array<SceneContentNodePtr>& children() const { return m_children; }

    // child nodes that are entities (helper)
    INLINE const Array<SceneContentEntityNodePtr>& entities() const { return m_entities; }

    // child nodes that are components (helper)
    INLINE const Array<SceneContentBehaviorNodePtr>& behaviors() const { return m_behaviors; }

    // visual flags
    INLINE SceneContentNodeVisualFlags visualFlags() const { return m_visualFlags; }

    // model index for tree view
    INLINE uint64_t uniqueModelIndex() const { return m_uniqueModelIndex; }

    // was the node marked as modified
    INLINE bool modified() const { return m_modified; }

    // dirty flags - what has changed since last sync
    INLINE SceneContentNodeDirtyFlags dirtyFlags() const { return m_dirtyFlags; }

    //--

    // can we attach children of given type
    virtual bool canAttach(SceneContentNodeType type) const = 0;

    // can we delete this node
    virtual bool canDelete() const = 0;

    // can we copy this node
    virtual bool canCopy() const = 0;

    //--

    // attach a child node, attached node must be "free floating" but this node can be either attached or not
    void attachChildNode(SceneContentNode* child);

    // detach a previously attached node
    void detachChildNode(SceneContentNode* child);

    // detach all children nodes
    void detachAllChildren();

    // recalculate merged visibility state for this node an all child nodes
    void recalculateVisibility();

    // change visual flags (just used for tree visualization)
    void visualFlag(SceneContentNodeVisualBit flag, bool value);

    // check if node is under hierarchy of given node
    bool contains(SceneContentNode* node) const;

    // bind root structure
    void bindRootStructure(SceneContentStructure* structure);

    // mark node (and children as not modified)
    void resetModifiedStatus(bool recursive = true);

    // reset dirty status of node
    void resetDirtyStatus(SceneContentNodeDirtyFlags flags);

    // report dirty content of some sorts that will require sync with visual 
    void markDirty(SceneContentNodeDirtyFlags flags);

    // find child node by name
    SceneContentNode* findChild(StringView name) const;

    // find node by given path - can contain '..' to go to parent
    SceneContentNode* findNodeByPath(StringView path) const;

    //--

    // change node local visibility (may propagate to other nodes if we are part of active structure)
    void visibility(SceneContentNodeLocalVisibilityState flag, bool propagateState = true);

    // visibility flag for the node - uses default visibility state unless overridden
    bool visibilityFlagBool() const;

    // get node's default visibility state
    virtual bool defaultVisibilityFlag() const;

    //--

    // generate unique node name within avoiding the ones in the "takenNames"
    static StringBuf BuildUniqueName(StringView coreName, bool userGiven, const HashSet<StringBuf>& takenNames);

    // generate unique CHILD node name, can additionally avoid names from the taken list
    StringBuf buildUniqueName(StringView coreName, bool userGiven = false, const HashSet<StringBuf>* additionalTakenName=nullptr) const;

    // change node name
    void name(const StringBuf& name);

    // get the UI icon for given node type
    static StringView IconTextForType(SceneContentNodeType type);

    //--

    // collect parent nodes (roots first)
    void collectHierarchyNodes(Array<const SceneContentNode*>& outNodes) const;

    // check if this node contains given node down the hierarchy (returns true if both are the same as well)
    bool contains(const SceneContentNode* node) const;

    // build a string representing node full path
    StringBuf buildHierarchicalName() const; // /world/layers/crap/entity

    //--

    // enumerate classes of entities that can be initialized from a given primary resource
    static void EnumEntityClassesForResource(ClassType resourceClass, Array<SpecificClassType<Entity>>& outEntityClasses);

    //--

    // HACK: selection filtering flag, REMOVE
    bool tempFlag = false;

    //--

    virtual void handleChildAdded(SceneContentNode* child);
    virtual void handleChildRemoved(SceneContentNode* child);
    virtual void handleParentChanged();
    virtual void handleLocalVisibilityChanged();
    virtual void handleVisibilityChanged();
    virtual void handleDebugRender(rendering::FrameParams& frame) const;

    //--

    virtual void displayText(IFormatStream& f) const;
    virtual void markModified() override;

    //--

private:
    SceneContentStructure* m_structure = nullptr;

    bool m_modified = false;
    bool m_visible = true;
    SceneContentNodeLocalVisibilityState m_localVisibilityFlag = SceneContentNodeLocalVisibilityState::Default;
    SceneContentNodeType m_type = SceneContentNodeType::None;

    Array<SceneContentNodePtr> m_children; // objects just in this file
    Array<SceneContentEntityNodePtr> m_entities; // specific list of just entities
    Array<SceneContentBehaviorNodePtr> m_behaviors; // specific list of just scripts

    SceneContentNodeVisualFlags m_visualFlags;
    SceneContentNodeDirtyFlags m_dirtyFlags;

    StringBuf m_name;

    uint64_t m_uniqueModelIndex = 0;

    mutable HashSet<StringBuf> m_childrenNames;
        
    //--

    void propagateMergedVisibilityState(bool parentVisibilityState);
    void propagateMergedVisibilityStateFromThis();

    void conditionalAttachToStructure();
    void conditionalDetachFromStructure();
    void conditionalChangeModifiedStatus(bool newStatus);
};

//--

/// general scene node for root of a world structure
class EDITOR_SCENE_EDITOR_API SceneContentWorldRoot : public SceneContentNode
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneContentWorldRoot, SceneContentNode);

public:
    SceneContentWorldRoot();

    virtual bool canAttach(SceneContentNodeType type) const override final;
    virtual bool canDelete() const override final;
    virtual bool canCopy() const override final;
};

//--

/// general scene node that maps to a file on disk
class EDITOR_SCENE_EDITOR_API SceneContentWorldLayer : public SceneContentNode
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneContentWorldLayer, SceneContentNode);

public:
    SceneContentWorldLayer(const StringBuf& name);

    virtual bool canAttach(SceneContentNodeType type) const override final;
    virtual bool canDelete() const override final;
    virtual bool canCopy() const override final;
};

//--

/// general scene node that maps to a directory on disk
class EDITOR_SCENE_EDITOR_API SceneContentWorldDir : public SceneContentNode
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneContentWorldDir, SceneContentNode);

public:
    SceneContentWorldDir(const StringBuf& name, bool system);

    virtual bool canAttach(SceneContentNodeType type) const override final;
    virtual bool canDelete() const override final;
    virtual bool canCopy() const override final;

    virtual bool defaultVisibilityFlag() const override;

private:
    bool m_systemDirectory = false;
};

//--

/// general scene node for root of a prefab structure
class EDITOR_SCENE_EDITOR_API SceneContentPrefabRoot : public SceneContentNode
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneContentPrefabRoot, SceneContentNode);

public:
    SceneContentPrefabRoot();

    virtual bool canAttach(SceneContentNodeType type) const override final;
    virtual bool canDelete() const override final;
    virtual bool canCopy() const override final;
};

//--

/// general node with editable content (component/entity)
class EDITOR_SCENE_EDITOR_API SceneContentDataNode : public SceneContentNode
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneContentDataNode, SceneContentNode);

public:
    SceneContentDataNode(SceneContentNodeType nodeType, const StringBuf& name, const ObjectIndirectTemplate* editableData);

    // do we have override content defined at this node (coming from prefab)
    INLINE bool hasBaseContent() const { return !m_baseData.empty(); }

    // base data of this node, always valid if we come from prefab - NOTE: we might have more than one base node (if so, data is stacked)
    INLINE const Array<ObjectIndirectTemplatePtr>& baseData() const { return m_baseData; }

    // editable data of this node, always valid, stacked on top of any base nodes
    INLINE const ObjectIndirectTemplatePtr& editableData() const { return m_editableData; }

    // get current local to parent transform (it's always the placement from editable data)
    INLINE const EulerTransform& localToParent() const { return m_localToWorld; }

    // get the cached "local to world" for this node
    INLINE const AbsoluteTransform& cachedLocalToWorldTransform() const { return m_cachedLocalToWorldPlacement; }

    // get the cached "local to world" for this node
    INLINE const Matrix& cachedLocalToWorldMatrix() const { return m_cachedLocalToWorldMatrix; }

    //--

    // get parent transform
    const AbsoluteTransform& cachedParentToWorldTransform() const;

    // change local to parent placement
    // NOTE: placement of child nodes is NOT changed
    void changeLocalPlacement(const EulerTransform& localToParent, bool force = false);

    // change class of data
    void changeClass(ClassType templateClass);

    // determine data class
    ClassType dataClass() const;

    // flatten template data
    ObjectIndirectTemplatePtr compileFlatData() const;

    //--

protected:
    void cacheTransformData();
    void updateBaseTemplates(const Array<const ObjectIndirectTemplate*>& baseTemplates);

    virtual void handleTransformUpdated();
    virtual void handleDebugRender(rendering::FrameParams& frame) const override;
    virtual void handleParentChanged() override;
    virtual void handleDataPropertyChanged(const StringBuf& data);
    virtual void displayText(IFormatStream& txt) const override;

private:
    AbsoluteTransform m_cachedLocalToWorldPlacement; // local to world placement for the node
    Matrix m_cachedLocalToWorldMatrix; // changed whenever parent changes as well

    ObjectIndirectTemplatePtr m_editableData; // always valid
    InplaceArray<ObjectIndirectTemplatePtr, 2> m_baseData; // valid only if we are instanced from a prefab
    EulerTransform m_baseLocalToTransform;

    EulerTransform m_localToWorld; // editable + base

    GlobalEventTable m_dataEvents;
};

//--

struct SceneContentEntityInstancedContent
{
    Array<NodeTemplatePrefabSetup> localPrefabs;
    Array<SceneContentNodePtr> childNodes;
};

/// general entity node (always in a file, can be parented to other entity)
class EDITOR_SCENE_EDITOR_API SceneContentEntityNode : public SceneContentDataNode
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneContentEntityNode, SceneContentDataNode);

public:
    SceneContentEntityNode(const StringBuf& name, const NodeTemplatePtr& node, const Array<NodeTemplatePtr>& inheritedTemplated = Array<NodeTemplatePtr>());

    INLINE const Array<NodeTemplatePrefabSetup>& localPrefabs() const { return m_localPrefabs; }

    NodeTemplatePtr compileDifferentialData() const;

    NodeTemplatePtr compileSnapshot() const;

    NodeTemplatePtr compiledForCopy() const;

    void invalidateData();

    //--

    void extractCurrentInstancedContent(SceneContentEntityInstancedContent& outInstancedContent); // NOTE: destructive !
    void createInstancedContent(const NodeTemplate* dataTemplate, SceneContentEntityInstancedContent& outInstancedContent) const;
    void applyInstancedContent(const SceneContentEntityInstancedContent& content);

    //--

    virtual bool canAttach(SceneContentNodeType type) const override final;
    virtual bool canDelete() const override final;
    virtual bool canCopy() const override final;

    bool canExplodePrefab() const;

private:
    Array<NodeTemplatePtr> m_inheritedTemplates;
    Array<NodeTemplatePrefabSetup> m_localPrefabs;

    bool m_dirtyVisualData = false;
    bool m_dirtyVisualTransform = false;

    virtual void handleDataPropertyChanged(const StringBuf& data) override;
    virtual void handleChildAdded(SceneContentNode* child) override;
    virtual void handleChildRemoved(SceneContentNode* child) override;
    virtual void handleVisibilityChanged() override;

    void updateBaseTemplates();
    void collectBaseNodes(const Array<NodeTemplatePrefabSetup>& localPrefabs, Array<const NodeTemplate*>& outBaseNodes) const;
};

//--

/// general behavior node 
class EDITOR_SCENE_EDITOR_API SceneContentBehaviorNode : public SceneContentDataNode
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneContentBehaviorNode, SceneContentDataNode);

public:
    SceneContentBehaviorNode(const StringBuf& name, const ObjectIndirectTemplate* editableData, const Array<const ObjectIndirectTemplate*>& baseData = Array<const ObjectIndirectTemplate*>());

    virtual bool canAttach(SceneContentNodeType type) const override final;
    virtual bool canDelete() const override final;
    virtual bool canCopy() const override final;

protected:
    virtual void handleDataPropertyChanged(const StringBuf& data) override;
    virtual void handleTransformUpdated() override;
    virtual void handleLocalVisibilityChanged() override;
};

//--

END_BOOMER_NAMESPACE_EX(ed)
