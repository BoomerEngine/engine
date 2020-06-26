/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: screens #]
***/

#include "build.h"
#include "gameHost.h"
#include "gameScreen_MeshViewer.h"
#include "rendering/texture/include/renderingTexture.h"
#include "rendering/driver/include/renderingShaderLibrary.h"
#include "rendering/driver/include/renderingCommandWriter.h"
#include "rendering/driver/include/renderingFramebuffer.h"
#include "rendering/scene/include/renderingScene.h"
#include "rendering/scene/include/renderingSceneProxyDesc.h"
#include "rendering/mesh/include/renderingMesh.h"
#include "rendering/scene/include/renderingFrameCamera.h"
#include "rendering/scene/include/renderingFrameParams.h"
#include "rendering/scene/include/renderingFrameRenderingService.h"
#include "rendering/scene/include/renderingFrameCameraContext.h"
#include "base/input/include/inputStructures.h"

namespace game
{
    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(Screen_MeshViewer);
        RTTI_PROPERTY(m_mesh);
    RTTI_END_TYPE();

    Screen_MeshViewer::Screen_MeshViewer(const rendering::MeshRef& mesh)
        : m_mesh(mesh)
    {
    }

    Screen_MeshViewer::Screen_MeshViewer(const base::res::ResourcePath& path)
    {
        startAsyncLoading(path);
    }

    Screen_MeshViewer::~Screen_MeshViewer()
    {
        destroyVis();
    }    

    void Screen_MeshViewer::startAsyncLoading(const base::res::ResourcePath& path)
    {
        auto selfRef = base::RefWeakPtr<Screen_MeshViewer>(this);
        RunFiber("LoadPreviewResource") << [selfRef, path](FIBER_FUNC)
        {
            if (auto mesh = base::LoadResource<rendering::Mesh>(path))
            {
                RunSync("ApplyLoadedPreviewResource") << [selfRef, mesh](FIBER_FUNC)
                {
                    if (auto self = selfRef.lock())
                        self->configureWithMesh(mesh);
                };
            }
        };
    }

    void Screen_MeshViewer::configureWithMesh(const rendering::MeshRef& mesh)
    {
        bool hasVis = m_scene;

        if (hasVis)
            destroyVis();

        m_mesh = mesh;

        if (hasVis)
            createVis();
    }

    bool Screen_MeshViewer::handleReadyCheck()
    {
        return m_mesh;
    }

    ScreenTransitionRequest Screen_MeshViewer::handleUpdate(double dt)
    {
        m_camera.animate(dt); 
        m_internalTimer += dt;
        return TBaseClass::handleUpdate(dt);
    }

    void Screen_MeshViewer::handleRender(rendering::command::CommandWriter& cmd, const HostViewport& hostViewport)
    {
        // compute camera
        rendering::scene::CameraSetup cameraSetup;
        cameraSetup.aspect = hostViewport.width / (float)hostViewport.height;
        m_camera.computeRenderingCamera(cameraSetup);

        // create camera
        rendering::scene::Camera camera;
        camera.setup(cameraSetup);

        // create frame and render scene content into it
        rendering::scene::FrameParams frame(hostViewport.width, hostViewport.height, camera);
        frame.index = m_frameIndex++;
        //frame.filters = m_filterFlags;
        frame.time.engineRealTime = (float)m_internalTimer;
        frame.time.gameTime = (float)m_internalTimer;

        // render the scene
        auto& sceneInfo = frame.scenes.scenesToDraw.emplaceBack();
        sceneInfo.scenePtr = m_scene;

        // generate command buffers
        if (auto* commandBuffer = base::GetService<rendering::scene::FrameRenderingService>()->renderFrame(frame, hostViewport.backBufferColor))
            cmd.opAttachChildCommandBuffer(commandBuffer);
    }

    bool Screen_MeshViewer::handleInput(const base::input::BaseEvent& evt)
    {
        if (const auto* key = evt.toKeyEvent())
        {
            if (m_camera.processKeyEvent(*key))
                return true;
        }
        else if (const auto* mouse = evt.toMouseMoveEvent())
        {
            m_camera.processMouseEvent(*mouse);
        }

        return false;
    }

    void Screen_MeshViewer::createVis()
    {
        // create the scene
        rendering::scene::SceneSetupInfo setup;
        setup.type = rendering::scene::SceneType::EditorPreview;
        m_scene = base::CreateSharedPtr<rendering::scene::Scene>(setup);

        // create the persistent context for camera (allows luminance adaptation, TAA, etc)
        m_cameraContext = base::CreateSharedPtr<rendering::scene::CameraContext>();

        // create mes proxy
        if (auto mesh = m_mesh.acquire())
        {
            rendering::scene::ProxyMeshDesc desc;
            desc.mesh = mesh;
            m_proxy = m_scene->proxyCreate(desc);
        }
    }

    void Screen_MeshViewer::destroyVis()
    {
        if (m_proxy)
        {
            m_scene->proxyDestroy(m_proxy);
            m_proxy.reset();
        }

        m_scene.reset();
        m_cameraContext.reset();
    }

    void Screen_MeshViewer::handleAttach()
    {       
        createVis();
        TBaseClass::handleAttach(); // starts a fade in
    }

    void Screen_MeshViewer::handleDetach()
    {
        destroyVis();
        TBaseClass::handleDetach();
    }
    
    //--

} // game


