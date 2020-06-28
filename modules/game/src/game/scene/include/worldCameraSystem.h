/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\systems #]
***/

#pragma once

#include "worldSystem.h"
#include "gameCameraComponent.h"

namespace game
{
    //---

    /// camera system - contains stack of cameras
    class GAME_SCENE_API WorldCameraSystem : public IWorldSystem
    {
        RTTI_DECLARE_VIRTUAL_CLASS(WorldCameraSystem, IWorldSystem);

    public:
        WorldCameraSystem();
        virtual ~WorldCameraSystem();

        //--

        /// push camera component on stack, if camera already exists and forcePush is not set to true than we just keep it there
        void pushCamera(CameraComponent* camera, bool forcePush=true);

        /// pop camera from stack
        void popCamera(CameraComponent* camera);

        //--

    protected:
        // IWorldSystem
        virtual bool handleInitialize(World& scene) override;
        virtual void handleShutdown(World& scene) override;
        virtual void handlePrepareCamera(World& scene, rendering::scene::CameraSetup& outCamera);

        //--

        struct CameraEntry
        {
            base::RefWeakPtr<CameraComponent> camera;
        };

        base::Array<CameraEntry> m_cameraStack;
        CameraParameters m_lastSetup;

        //---
    };

    //---

} // game