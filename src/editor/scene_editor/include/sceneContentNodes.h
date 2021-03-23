/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\structure #]
***/

#pragma once

#include "engine/world/include/rawEntity.h"
#include "engine/ui/include/uiTreeViewEx.h"

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

    /*// child nodes that are components (helper)
    INLINE const Array<SceneContentBehaviorNodePtr>& behaviors() const { return m_behaviors; }*/

    // visual flags
    INLINE SceneContentNodeVisualFlags visualFlags() const { return m_visualFlags; }

    // was the node marked as modified
    INLINE bool modified() const { return m_modified; }

    // dirty flags - what has changed since last sync
    INLINE SceneContentNodeDirtyFlags dirtyFlags() const { return m_dirtyFlags; }

    // tree element (for tree view)
    INLINE SceneContentTreeItem* treeItem() const { return m_treeItem; }

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

    virtual void markModified() override;

    //--

    virtual void displayTags(IFormatStream& f) const;

    void updateDisplayElement();

private:
    SceneContentStructure* m_structure = nullptr;

    bool m_modified = false;
    bool m_visible = true;
    SceneContentNodeLocalVisibilityState m_localVisibilityFlag = SceneContentNodeLocalVisibilityState::Default;
    SceneContentNodeType m_type = SceneContentNodeType::None;

    Array<SceneContentNodePtr> m_children; // objects just in this file
    Array<SceneContentEntityNodePtr> m_entities; // specific list of just entities
    //Array<SceneContentBehaviorNodePtr> m_behaviors; // specific list of just scripts

    SceneContentNodeVisualFlags m_visualFlags;
    SceneContentNodeDirtyFlags m_dirtyFlags;

    StringBuf m_name;

    mutable HashSet<StringBuf> m_childrenNames;

    SceneContentTreeItemPtr m_treeItem;
        
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

END_BOOMER_NAMESPACE_EX(ed)
