/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: node #]
***/

#pragma once

namespace ed
{
    //--

    DECLARE_GLOBAL_EVENT(EVENT_EDITOR_CHILD_NODE_ATTACHED, EditorNodePtr); // pointer to node attached
    DECLARE_GLOBAL_EVENT(EVENT_EDITOR_CHILD_NODE_DETACHED, EditorNodePtr); // pointer to node detaching
    DECLARE_GLOBAL_EVENT(EVENT_EDITOR_NODE_NAME_CHANGED, EditorNodePtr); // pointer to node that changes name
    DECLARE_GLOBAL_EVENT(EVENT_EDITOR_NODE_VISIBILITY_CHANGED, EditorNodePtr); // pointer to node that changed it's visibility state

    //--

    /// transform flags
    enum class EditorNodeTransformFlagBit : uint8_t
    {
        Identity = FLAG(0),
        HasScale = FLAG(1),
        HasNonUniformScale = FLAG(2),
        FlipFaces = FLAG(3),

        LocalToWorldCached = FLAG(6),
        WorldToLocalCached = FLAG(7),
    };

    typedef DirectFlags<EditorNodeTransformFlagBit> EditorNodeTransformFlags;

    /// transform of a scene node
    /// NOTE: transform are NOT a tree they are always final placements, only when nodes are acted upon (ie. moved, etc) we figure out how they are grouped
    class ASSETS_SCENE_COMMON_API EditorNodeTransform : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(EditorNodeTransform, IObject);

    public:
        EditorNodeTransform();
        virtual ~EditorNodeTransform();

        //--

        // get the owning node
        INLINE EditorNode* node() const { return rtti_cast<EditorNode>(parent()); }

        // raw position
        INLINE const AbsolutePosition& position() const { return m_position; }

        // raw rotation
        INLINE const Angles& rotation() const { return m_rotation; }
        
        // raw scale
        INLINE const Vector3& scale() const { return m_scale; }

        //--

        // flags
        INLINE EditorNodeTransformFlags flags() const { return m_flags; }

        // is this an identity transform ?
        INLINE bool identity() const { return m_flags.test(EditorNodeTransformFlagBit::Identity); }

        // do we have scaling set ?
        INLINE bool hasScale() const { return m_flags.test(EditorNodeTransformFlagBit::HasScale); }

        // do we have non uniform scale set ?
        INLINE bool hasNonUniformScale() const { return m_flags.test(EditorNodeTransformFlagBit::HasNonUniformScale); }

        // does the whole transform as a whole flip faces ?
        INLINE bool flipsFaces() const { return m_flags.test(EditorNodeTransformFlagBit::FlipFaces); }

        //--

        // convert to a absolute placement transform
        AbsoluteTransform toTransform() const;

        // get local space -> world space matrix, useful for quick operations, not 100% accurate
        const Matrix& evalLocalToWorld();

        // get world space -> local space matrix, useful for quick operations, not 100% accurate
        const Matrix& evalWorldToLocal();

        //--

        // set new transform
        void setup(const AbsolutePosition& position, const Angles& rotation = Angles::ZERO(), const Vector3& scale = Vector3::ONE());

        // change position only
        void position(const AbsolutePosition& position);
        
        // change rotation only
        void rotation(const Angles& rotation);
        
        // change scale only
        void scale(const Vector3& scale);

        //--

    protected:
        EditorNodeTransformFlags m_flags;

        AbsolutePosition m_position;
        Angles m_rotation;
        Vector3 m_scale;

        Matrix m_localToWorld;
        Matrix m_worldToLocal;

        void invalidateMatrices();

        virtual void onPostLoad() override;
    };

    //--

    // pivot of a node
    class ASSETS_SCENE_COMMON_API EditorNodePivot : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(EditorNodePivot, IObject);

    public:
        EditorNodePivot();
        virtual ~EditorNodePivot();

        // get the owning node
        INLINE EditorNode* node() const { return rtti_cast<EditorNode>(parent()); }

        // pivot position in local object space
        INLINE const Vector3& position() const { return m_position; }

        // raw rotation
        INLINE const Angles& rotation() const { return m_rotation; }

    private:
        Vector3 m_position;
        Angles m_rotation;
    };

    //--

    /// context-less node in the scene
    /// NOTE: those nodes are part of undo/redo mechanic and can live after they are "destroyed"
    class ASSETS_SCENE_COMMON_API EditorNode : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(EditorNode, IObject);

    public:
        EditorNode();
        virtual ~EditorNode();

        //--

        // name of the node (can be empty for helper nodes)
        INLINE const StringBuf& name() const { return m_name; }

        // get parent node
        INLINE EditorNode* parent() const { return rtti_cast<EditorNode>(IObject::parent()); }

        // node transform
        INLINE const EditorNodeTransformPtr& transform() const { return m_transform; }

        // node pivot (used ONLY when moving/rotation node with 3D gizmo)
        INLINE const EditorNodePivotPtr& pivot() const { return m_pivot; }

        // children - nodes moved 
        INLINE const base::Array<EditorNodePtr>& children() const { return m_children; }

        // scene structure we belong to
        INLINE EditorScene* scene() const { return m_scene; }

        // runtime flag - is the node selected
        INLINE bool selected() const { return m_selected; }

        // runtime flag - is the node visible
        INLINE bool localVisible() const { return m_visibleLocal; }

        // runtime flag - is the node and all parents visible
        INLINE bool mergedVisible() const { return m_visibleMerged; }

        //--

        /// attach child node to hierarchy
        void attachChild(EditorNode* node);

        /// detach child node from hierarchy
        void detachChild(EditorNode* node);

        /// attach node to scene
        void attachScene(EditorScene* scene);

        /// detach node from scene
        void detachScene(EditorScene* scene);

        /// update node
        void update(float dt);

        /// render node, called only for visible nodes
        void render(rendering::scene::FrameParams& params);

        /// change node visibility
        void visibility(bool visible);

        //--

        /// get data view to editable content
        virtual DataViewPtr contentDataView();

        /// update node, called only if node is part of attached scene
        virtual void handleUpdate(float dt);

        /// child got attached
        virtual void handleChildAttached(EditorNode* child);

        /// child got detached
        virtual void handleChildDetached(EditorNode* child);

        /// node got attached to scene structure, good place to create visualizations
        virtual void handleSceneAttached(EditorScene* scene);

        /// node is being detached from scene structure (remove visualizations)
        virtual void handleSceneDetached(EditorScene* scene);

        /// node transform has changed
        virtual void handleUpdateTransform();

        /// create node visualization, called when we are attached to scene and visible
        virtual void handleCreateVisualization();

        /// create node visualization, called when we are attached to scene and visible
        virtual void handleDestroyVisualization();

        //--

    protected:
        void updateVisibility();
         
    private:
        EditorNodeTransformPtr m_transform;
        EditorNodePivotPtr m_pivot;        
        Array<EditorNodePtr> m_children;

        bool m_selected : 1;
        bool m_visibleLocal : 1;
        bool m_visibleMerged : 1;

        StringBuf m_name;

        EditorScene* m_scene = nullptr;
    };

    //--

} // ed
