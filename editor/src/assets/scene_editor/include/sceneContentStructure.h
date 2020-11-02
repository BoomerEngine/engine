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

    /// structure of the edited scene, contains attached nodes as well as visualization helpers
    class ASSETS_SCENE_EDITOR_API SceneContentStructure : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneContentStructure, IObject);

    public:
        SceneContentStructure(const game::WorldPtr& previewWorld, SceneContentNode* rootNode); // usually the root is either WorldRoot or PrefabRoot
        virtual ~SceneContentStructure();

        // the root node
        INLINE const SceneContentNodePtr& root() const { return m_root; }

        // all attached entity nodes (regardless of visibility settings), parents first
        INLINE const Array<SceneContentEntityNodePtr>& entities() const { return m_entities; }

        // all attached nodes, parents first
        INLINE const Array<SceneContentNodePtr>& nodes() const { return m_nodes; }

        // preview world where all the visualizations are created
        INLINE const game::WorldPtr& previewWorld() const { return m_previewWorld; }

        //--

        // render in-editor debug representation of entities
        void handleDebugRender(rendering::scene::FrameParams& frame) const;

        //--

        // inform about node being added to structure
        void handleNodeAdded(SceneContentNode* node);

        // inform about node being removed to structure
        void handleNodeRemoved(SceneContentNode* node);

        //--

    private:
        SceneContentNodePtr m_root;

        Array<SceneContentEntityNodePtr> m_entities;
        Array<SceneContentNodePtr> m_nodes;

        game::WorldPtr m_previewWorld;
    };

    //--

} // ed
