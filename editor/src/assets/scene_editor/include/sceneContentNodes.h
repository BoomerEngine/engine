/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\structure #]
***/

#pragma once

namespace ed
{

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
    class ASSETS_SCENE_EDITOR_API SceneContentNode : public IObject
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

        // is the node visible ?
        INLINE bool visible() const { return m_visible; }

        // child nodes
        INLINE const Array<SceneContentNodePtr>& children() const { return m_children; }

        // child nodes that are entities (helper)
        INLINE const Array<SceneContentEntityNodePtr>& entities() const { return m_entities; }

        // child nodes that are components (helper)
        INLINE const Array<SceneContentComponentNodePtr>& components() const { return m_components; }

        // visual flags
        INLINE SceneContentNodeVisualFlags visualFlags() const { return m_visualFlags; }

        // model index for tree view
        INLINE uint64_t uniqueModelIndex() const { return m_uniqueModelIndex; }

        // was the node marked as modified
        INLINE bool modified() const { return m_modified; }

        // dirty flags - what has changed since last sync
        INLINE SceneContentNodeDirtyFlags dirtyFlags() const { return m_dirtyFlags; }

        //--

        // attach a child node, attached node must be "free floating" but this node can be either attached or not
        void attachChildNode(SceneContentNode* child);

        // detach a previously attached node
        void detachChildNode(SceneContentNode* child);

        // detach all children nodes
        void detachAllChildren();

        // change node local visibility (may propagate to other nodes if we are part of active structure)
        void visibility(bool flag);

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

        //--

        virtual void handleChildAdded(SceneContentNode* child);
        virtual void handleChildRemoved(SceneContentNode* child);
        virtual void handleParentChanged();
        virtual void handleVisibilityChanged();
        virtual void handleDebugRender(rendering::scene::FrameParams& frame) const;

        //--

        virtual void displayText(IFormatStream& f) const;
        virtual void markModified() override;

        StringBuf buildUniqueName(StringView<char> coreName, bool userGiven = false, const HashSet<StringBuf>* additionalTakenName=nullptr) const;

        void name(const StringBuf& name);

        //--

    private:
        SceneContentStructure* m_structure = nullptr;

        bool m_modified = false;
        bool m_visible = true;
        bool m_localVisibilityFlag = true;
        SceneContentNodeType m_type = SceneContentNodeType::None;

        Array<SceneContentNodePtr> m_children; // objects just in this file
        Array<SceneContentEntityNodePtr> m_entities; // specific list of just entities
        Array<SceneContentComponentNodePtr> m_components; // specific list of just components

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
    class ASSETS_SCENE_EDITOR_API SceneContentWorldRoot : public SceneContentNode
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneContentWorldRoot, SceneContentNode);

    public:
        SceneContentWorldRoot();
    };

    //--

    /// general scene node that maps to a file on disk
    class ASSETS_SCENE_EDITOR_API SceneContentWorldLayer : public SceneContentNode
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneContentWorldLayer, SceneContentNode);

    public:
        SceneContentWorldLayer(const StringBuf& name);
    };

    //--

    /// general scene node that maps to a directory on disk
    class ASSETS_SCENE_EDITOR_API SceneContentWorldDir : public SceneContentNode
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneContentWorldDir, SceneContentNode);

    public:
        SceneContentWorldDir(const StringBuf& name);
    };

    //--

    /// general scene node for root of a prefab structure
    class ASSETS_SCENE_EDITOR_API SceneContentPrefabRoot : public SceneContentNode
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneContentPrefabRoot, SceneContentNode);

    public:
        SceneContentPrefabRoot();
    };

    //--

    /// general node with editable content (component/entity)
    class ASSETS_SCENE_EDITOR_API SceneContentDataNode : public SceneContentNode
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneContentDataNode, SceneContentNode);

    public:
        SceneContentDataNode(SceneContentNodeType nodeType, const StringBuf& name, const EulerTransform& localToParent, const ObjectTemplatePtr& editableData, const ObjectTemplatePtr& baseData=nullptr); // NOTE: objects ownership is changed

        // do we have override content defined at this node (coming from prefab)
        INLINE bool hasBaseContent() const { return !!m_baseData; }

        // editable data of this node, always valid
        INLINE const ObjectTemplatePtr& editableData() const { return m_editableData; }

        // base data of this node, always valid
        INLINE const ObjectTemplatePtr& baseData() const { return m_baseData; }

        // get the placement between this node and the parent node
        INLINE const EulerTransform& localToParent() const { return m_localToParent; }

        // get the cached "local to world" for this node
        INLINE const AbsoluteTransform& cachedLocalToWorldTransform() const { return m_cachedLocalToWorldTransform; }

        // get the cached "local to world" for this node
        INLINE const Matrix& cachedLocalToWorldMatrix() const { return m_cachedLocalToWorldMatrix; }

        //--

        // change placement of the node in local mode (relative to parent)
        // this is the most "normal" mode
        void changeLocalPlacement(const EulerTransform& newTramsform, bool force = false);

        // change data object hold at node
        void changeData(const ObjectTemplatePtr& data);

        //--

    protected:
        void updateTransform(bool force = false, bool recursive = true);
        bool cacheTransformData();

        virtual void handleTransformUpdated();
        virtual void handleDebugRender(rendering::scene::FrameParams& frame) const override;
        virtual void handleParentChanged() override;
        virtual void handleDataPropertyChanged(const StringBuf& data);
        virtual void displayText(IFormatStream& txt) const override;

    private:
        EulerTransform m_localToParent;

        AbsoluteTransform m_cachedLocalToWorldTransform; // changed whenever parent changes as well
        Matrix m_cachedLocalToWorldMatrix; // changed whenever parent changes as well

        ObjectTemplatePtr m_editableData; // always valid
        ObjectTemplatePtr m_baseData; // valid only if we are instanced from a prefab

        GlobalEventTable m_dataEvents;
    };

    //--

    class ASSETS_SCENE_EDITOR_API SceneContentEntityNodePrefabSource : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneContentEntityNodePrefabSource, IObject);

    public:
        SceneContentEntityNodePrefabSource(const base::world::PrefabRef& prefab, bool enabled, bool inherited);

        INLINE const base::world::PrefabRef& prefab() const { return m_prefab; }

        INLINE bool enabled() const { return m_enabled; }

        INLINE bool inherited() const { return m_inherited; }

    private:
        base::world::PrefabRef m_prefab;
        bool m_enabled = true;
        bool m_inherited = false;
    };

    //--

    /// general entity node (always in a file, can be parented to other entity)
    class ASSETS_SCENE_EDITOR_API SceneContentEntityNode : public SceneContentDataNode
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneContentEntityNode, SceneContentDataNode);

    public:
        SceneContentEntityNode(const StringBuf& name, Array<RefPtr<SceneContentEntityNodePrefabSource>>&& rootPrefabs, const base::world::EntityTemplatePtr& editableData, const base::world::EntityTemplatePtr& baseData = nullptr);

        INLINE const Array<RefPtr<SceneContentEntityNodePrefabSource>>& prefabs() const { return m_prefabAssets; }

        base::world::EntityTemplatePtr compileData() const;

        base::world::NodeTemplatePtr compileDifferentialData(bool& outAnyMeaningfulData) const;

        base::world::NodeTemplatePtr compileSnapshot() const;

    private:
        Array<RefPtr<SceneContentEntityNodePrefabSource>> m_prefabAssets;

        bool m_dirtyVisualData = false;
        bool m_dirtyVisualTransform = false;

        virtual void handleDataPropertyChanged(const StringBuf& data) override;
        virtual void handleChildAdded(SceneContentNode* child) override;
        virtual void handleChildRemoved(SceneContentNode* child) override;
    };

    //--

    /// general component node 
    class ASSETS_SCENE_EDITOR_API SceneContentComponentNode : public SceneContentDataNode
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneContentComponentNode, SceneContentDataNode);

    public:
        SceneContentComponentNode(const StringBuf& name, const base::world::ComponentTemplatePtr& editableData, const base::world::ComponentTemplatePtr& baseData = nullptr);

        base::world::ComponentTemplatePtr compileData() const;

        base::world::ComponentTemplatePtr compileDifferentialData(bool& outAnyMeaningfulData) const;

    protected:
        virtual void handleDataPropertyChanged(const StringBuf& data) override;
    };

    //--

    // unpack a node hierarchy into editable node structure
    // NOTE: this can be called only on non-instanced node data, usually the root node, otherwise it's meaningless
    extern ASSETS_SCENE_EDITOR_API SceneContentEntityNodePtr UnpackNode(const base::world::NodeTemplate* rootNode, Array<SceneContentEntityNodePtr>* outAllEntities = nullptr);

    //--

} // ed
