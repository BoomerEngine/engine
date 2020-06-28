/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
*
***/

#include "build.h"
#include "world.h"
#include "worldCameraSystem.h"
#include "gameCameraComponent.h"
#include "rendering/scene/include/renderingScene.h"
#include "rendering/scene/include/renderingFrameParams.h"

namespace game
{

    //---

    RTTI_BEGIN_TYPE_CLASS(WorldCameraSystem);
    RTTI_END_TYPE();

    WorldCameraSystem::WorldCameraSystem()
    {}

    WorldCameraSystem::~WorldCameraSystem()
    {}

    //--

    void WorldCameraSystem::pushCamera(CameraComponent* camera, bool forcePush)
    {
        if (camera)
        {
            // make sure camera is not added second time
            for (uint32_t i = 0; i < m_cameraStack.size(); ++i)
            {
                auto& entry = m_cameraStack[i];
                if (entry.camera == camera)
                {
                    if (!forcePush)
                        return;

                    m_cameraStack.erase(i);
                    break;
                }
            }

            // create entry
            auto& entry = m_cameraStack.emplaceBack();
            entry.camera = camera;
            //entry.m_lastPosition = camera->
        }
    }

    void WorldCameraSystem::popCamera(CameraComponent* camera)
    {
        for (uint32_t i = 0; i < m_cameraStack.size(); ++i)
        {
            auto& entry = m_cameraStack[i];
            if (entry.camera == camera)
            {
                m_cameraStack.erase(i);
                break;
            }
        }
    }

    //--

    bool WorldCameraSystem::handleInitialize(World& scene)
    {
        return true;
    }

    void WorldCameraSystem::handleShutdown(World& scene)
    {
        // nothing
    }

    void WorldCameraSystem::handlePrepareCamera(World& scene, rendering::scene::CameraSetup& outCamera)
    {
        // use first valid camera on the stack to calculate the parameters of the camera
        for (int i = m_cameraStack.lastValidIndex(); i >= 0; --i)
        {
            if (auto camera = m_cameraStack[i].camera.lock())
            {
                camera->computeCameraParams(m_lastSetup);
                break;
            }
            else
            {
                m_cameraStack.erase(i);
            }
        }

        // convert the game camera settings to rendering settings
        outCamera.position = m_lastSetup.position.approximate();
        outCamera.rotation = m_lastSetup.rotation;
        outCamera.fov = m_lastSetup.fov;

        // setup custom planes
        if (m_lastSetup.forceNearPlane > 0.0f)
            outCamera.nearPlane = m_lastSetup.forceNearPlane;
        if (m_lastSetup.forceFarPlane > 0.0f)
            outCamera.farPlane = m_lastSetup.forceFarPlane;
    }

    ///---

} // game

