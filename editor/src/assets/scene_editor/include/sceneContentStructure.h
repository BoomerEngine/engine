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

    enum class SceneContentSyncOpType : uint8_t
    {
        Added,
        Removed,
    };

    struct SceneContentSyncOp
    {
        SceneContentSyncOpType type;
        SceneContentNodePtr node;
    };

    //--

    /// structure of the edited scene, contains attached nodes as well as visualization helpers
    class ASSETS_SCENE_EDITOR_API SceneContentStructure : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneContentStructure, IObject);

    public:
        SceneContentStructure(SceneContentNodeType rootNodeType); // usually the root is either WorldRoot or PrefabRoot
        virtual ~SceneContentStructure();

        // the root node
        INLINE const SceneContentNodePtr& root() const { return m_root; }

        // all attached entity nodes (regardless of visibility settings), parents first
        INLINE const Array<SceneContentEntityNodePtr>& entities() const { return m_entities; }

        // all attached nodes, parents first
        INLINE const Array<SceneContentNodePtr>& nodes() const { return m_nodes; }

        //--

        // render in-editor debug representation of entities
        void handleDebugRender(rendering::scene::FrameParams& frame) const;

        //--

        // inform about node being added to structure
        void nodeAdded(SceneContentNode* node);

        // inform about node being removed to structure
        void nodeRemoved(SceneContentNode* node);

        // inform that node has requested dirty state to be set (some part of it's content has changed)
        void nodeDirtyContent(SceneContentNode* node);

        //--

        // get list of nodes added/removed since last sync
        void syncNodeChanges(Array<SceneContentSyncOp>& outOps);

        // visit all nodes marked as dirty, used to sync the preview state with content state
        // synced state should be cleared from the dirtyFlags bitfield
        void visitDirtyNodes(const std::function<void(const SceneContentNode*, SceneContentNodeDirtyFlags&)>& func);

        //--

        // find node by hierarchical path (/world/layers/env/street/lamp_01)
        const SceneContentNode* findNodeByPath(StringView path) const;

        //--

    private:
        SceneContentNodePtr m_root;

        Array<SceneContentEntityNodePtr> m_entities;
        Array<SceneContentNodePtr> m_nodes;

        Array<SceneContentSyncOp> m_nodeChanges;
        HashSet<SceneContentNodeWeakPtr> m_dirtyNodes;
    };

    //--

} // ed
