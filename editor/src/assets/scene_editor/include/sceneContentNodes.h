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
        SelectedNode = FLAG(1),
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

        // is the node visualized ?
        INLINE bool visualized() const { return m_visualizationCreated; }

        // child nodes
        INLINE const Array<SceneContentNodePtr>& children() const { return m_children; }

        // child nodes that are entities (helper)
        INLINE const Array<SceneContentEntityNodePtr>& entities() const { return m_entities; }

        // visual flags
        INLINE SceneContentNodeVisualFlags visualFlags() const { return m_visualFlags; }

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

        //--

        virtual void handleChildAdded(SceneContentNode* child);
        virtual void handleChildRemoved(SceneContentNode* child);
        virtual void handleAttachedToStructure();
        virtual void handleDetachedFromStructure();
        virtual void handleParentChanged();
        virtual void handleVisibilityChanged();
        virtual void handleCreateVisualization();
        virtual void handleDestroyVisualization();
        virtual void handleDebugRender(rendering::scene::FrameParams& frame) const;

        //--

        virtual void displayText(IFormatStream& f) const;

        StringBuf buildUniqueName(const StringBuf& name, bool userGiven = false, const HashSet<StringBuf>* additionalTakenName=nullptr) const;

        void name(const StringBuf& name);

        //--

    private:
        SceneContentStructure* m_structure = nullptr;

        bool m_visible = true;
        bool m_localVisibilityFlag = true;
        bool m_visualizationCreated = false;
        SceneContentNodeType m_type = SceneContentNodeType::None;

        Array<SceneContentNodePtr> m_children; // objects just in this file
        Array<SceneContentEntityNodePtr> m_entities; // specific list of just entities

        SceneContentNodeVisualFlags m_visualFlags;

        StringBuf m_name;

        mutable HashSet<StringBuf> m_childrenNames;
        
        //--

        void propagateMergedVisibilityState(bool parentVisibilityState);
        void propagateMergedVisibilityStateFromThis();

        void conditionalAttachToStructure();
        void conditionalDetachFromStructure();
        void conditionalCreateVisualization();
        void conditionalDestroyVisualization();
        void conditionalRecreateVisualization(bool force=false);
    };

    //--

    /// general scene node that maps to a file on disk
    class ASSETS_SCENE_EDITOR_API SceneContentFileNode : public SceneContentNode
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneContentFileNode, SceneContentNode);

    public:
        SceneContentFileNode(const StringBuf& name, ManagedFileNativeResource* file, bool prefab=false); // always valid

        INLINE ManagedFileNativeResource* file() const { return m_file; }

        void reloadContent();

    private:
        ManagedFileNativeResource* m_file = nullptr;

        
    };

    //--

    /// general scene node that maps to a directory on disk
    class ASSETS_SCENE_EDITOR_API SceneContentDirNode : public SceneContentNode
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneContentDirNode, SceneContentNode);

    public:
        SceneContentDirNode(const StringBuf& name);


    private:
    };

    //--

    /// general scene node for root of a prefab structure
    class ASSETS_SCENE_EDITOR_API SceneContentPrefabRootNode : public SceneContentFileNode
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneContentPrefabRootNode, SceneContentFileNode);

    public:
        SceneContentPrefabRootNode(ManagedFileNativeResource* prefabAssetFile);

    };

    //--

    /// general scene node for root of a world structure
    class ASSETS_SCENE_EDITOR_API SceneContentWorldRootNode : public SceneContentNode
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneContentWorldRootNode, SceneContentNode);

    public:
        SceneContentWorldRootNode();
    };

    //--

    /// entity data for scene entity node
    struct ASSETS_SCENE_EDITOR_API SceneContentEntityNodeData : public IReferencable
    {
        StringID name; // empty for entity
        ObjectTemplatePtr editableData; // may be empty if payload is not locally defined
        ObjectTemplatePtr baseData; // all base templates merged into one, can be empty if node is not coming from a prefab
    };

    typedef RefPtr<SceneContentEntityNodeData> SceneContentEntityNodeDataPtr;

    //--

    /// general entity node (always in a file, can be parented to other entity)
    class ASSETS_SCENE_EDITOR_API SceneContentEntityNode : public SceneContentNode
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneContentEntityNode, SceneContentNode);

    public:
        SceneContentEntityNode(const StringBuf& name, const Transform& localToParent, const Array<game::NodeTemplatePtr>& templates, bool originalContent=true);

        // is this an original content node - ie. the node actually created by user, not from a prefab
        INLINE bool originalContent() const { return m_originalContent; }

        // do we have local content defined at this node
        INLINE bool hasLocalContent() const { return m_hasLocalContent; }

        // do we have override content defined at this node (coming from prefab)
        INLINE bool hasBaseContent() const { return m_hasBaseContent; }

        // get the placement between this node and the parent node
        INLINE const base::Transform& localToParent() const { return m_localToParent; }

        // get the cached "local to world" for this node
        INLINE const AbsoluteTransform& cachedLocalToWorldTransform() const { return m_cachedLocalToWorldTransform; }

        // get the cached "local to world" for this node
        INLINE const Matrix& cachedLocalToWorldMatrix() const { return m_cachedLocalToWorldMatrix; }

        //--

        // get all editable data payloads
        INLINE const Array<SceneContentEntityNodeDataPtr>& payloads() const { return m_payloads; } // at least 1 - the entity

        // get the entity payload
        INLINE const SceneContentEntityNodeDataPtr& entityPayload() const { return m_payloads[0]; }

        // find data for given name
        SceneContentEntityNodeDataPtr findPayload(StringID name) const;

        // attach data payload, will trigger data recompilation
        void attachPayload(SceneContentEntityNodeData* payload);

        // detach data payload, will trigger data recompilation
        void detachPayload(SceneContentEntityNodeData* payload);

        // invalidate payload data - will trigger node visual recompilation
        void invalidatePayload();

        //--

        // change placement of the node in local mode (relative to parent)
        // this is the most "normal" mode
        void changeLocalPlacement(const Transform& newTramsform, bool force = false);

        //--

        // describe node
        virtual void displayText(IFormatStream& txt) const override;

    private:
        Transform m_localToParent;

        AbsoluteTransform m_cachedLocalToWorldTransform; // changed whenever parent changes as well
        Matrix m_cachedLocalToWorldMatrix; // changed whenever parent changes as well

        Array<SceneContentEntityNodeDataPtr> m_payloads; // data payloads

        bool m_hasLocalContent = false;
        bool m_hasBaseContent = false;
        bool m_originalContent = false;

        //--

        void refreshContentFlags();

        void updateTransform(bool force=false, bool recursive=true);
        bool cacheTransformData();

        virtual void handleTransformUpdated();

        virtual void handleCreateVisualization() override;
        virtual void handleDestroyVisualization() override;
        virtual void handleDebugRender(rendering::scene::FrameParams& frame) const override;
        virtual void handleParentChanged() override;

        static void CompilePayloads(const Array<game::NodeTemplatePtr>& templates, SceneContentEntityNode* owner, Array<SceneContentEntityNodeDataPtr>& outPayloads);
    };

    //--

    // unpack prefab into node structure
    extern ASSETS_SCENE_EDITOR_API void UnpackNodeContainer(const game::NodeTemplateContainer& nodeContainer, const Transform& additionalRootPlacement, Array<SceneContentEntityNodePtr>* outRootEntities, Array<SceneContentEntityNodePtr>* outAllEntities = nullptr);

    //--

} // ed
