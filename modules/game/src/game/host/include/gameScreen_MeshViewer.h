/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: screens #]
***/

#pragma once

#include "gameScreen.h"
#include "gameFlyCamera.h"

namespace game
{
    //--

    // simple screen for a simple mesh viewer
    class GAME_HOST_API Screen_MeshViewer : public IScreen
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Screen_MeshViewer, IScreen);

    public:
        Screen_MeshViewer(const rendering::MeshRef& mesh);
        Screen_MeshViewer(const base::res::ResourcePath& path); // will load asynchronously
        virtual ~Screen_MeshViewer();

    protected:
        virtual bool handleReadyCheck() override;
        virtual ScreenTransitionRequest handleUpdate(double dt) override;
        virtual void handleRender(rendering::command::CommandWriter& cmd, const HostViewport& viewport) override;
        virtual bool handleInput(const base::input::BaseEvent& evt) override;
        virtual void handleAttach() override;
        virtual void handleDetach() override;

    private:
        rendering::MeshRef m_mesh;
        rendering::scene::ScenePtr m_scene;
        rendering::scene::ProxyHandle m_proxy;

        FlyCamera m_camera;
        rendering::scene::CameraContextPtr m_cameraContext;

        uint32_t m_frameIndex = 0;
        float m_internalTimer = 0.0f;

        void createVis();
        void destroyVis();
        
        void startAsyncLoading(const base::res::ResourcePath& path);
        void configureWithMesh(const rendering::MeshRef& mesh);
    };

    //--

} // game
