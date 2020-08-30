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

    /// editor scene 
    class ASSETS_SCENE_COMMON_API EditorScene : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(EditorScene, IObject)

    public:
        EditorScene(bool fullScene = false);
        virtual ~EditorScene();

        //--

        // get the game world
        INLINE game::World* world() const { return m_world; }

        // all nodes in the scene
        INLINE const Array<EditorNodePtr>& nodes() const { return m_nodes; }

        //--

        // update all nodes
        void update(float dt);

        // render all debug states
        void render(rendering::scene::FrameParams& frame);

        //--

    private:
        Array<EditorNodePtr> m_nodes; // all nodes

        void attachNode(EditorNode* node);
        void detachNode(EditorNode* node);

        bool m_duringUpdate = false;

        game::WorldPtr m_world;
        game::EntityPtr m_cameraEntity;

        friend class EditorNode;
    };

    //--

} // ed
