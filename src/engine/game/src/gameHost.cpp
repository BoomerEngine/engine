/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "game.h"
#include "gameWorld.h"
#include "gameHost.h"

#include "gpu/device/include/renderingFramebuffer.h"
#include "gpu/device/include/renderingCommandWriter.h"
#include "engine/canvas/include/canvasService.h"
#include "engine/rendering/include/renderingFrameParams.h"
#include "engine/rendering/include/renderingFrameRenderingService.h"
#include "engine/rendering/include/renderingFrameCameraContext.h"
#include "gpu/device/include/renderingImage.h"
#include "gpu/device/include/renderingDeviceService.h"
#include "gpu/device/include/renderingDeviceApi.h"

#include "engine/canvas/include/canvas.h"
#include "engine/imgui/include/debugPageService.h"
#include "engine/imgui/include/imgui_integration.h"
#include "engine/world/include/world.h"

#include "core/app/include/launcherPlatform.h"
#include "core/input/include/inputStructures.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_ENUM(HostType);
    RTTI_ENUM_OPTION(Standalone);
    RTTI_ENUM_OPTION(InEditor);
    RTTI_ENUM_OPTION(Server);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(Host);
RTTI_END_TYPE();

Host::Host(HostType type, const GamePtr& game)
    : m_type(type)
    , m_game(game)
{
    m_imgui = new ImGui::ImGUICanvasHelper();
    m_gameAccumulatedTime = 0.0;

    m_loadingScreenVisibility = 1.0f; // start under black screen

    m_startTime.resetToNow();

    m_cameraContext = RefNew<rendering::CameraContext>();
}

Host::~Host()
{
    delete m_imgui;
    m_imgui = nullptr;
}
    
bool Host::update(double dt)
{ 
    PC_SCOPE_LVL0(GameHostUpdate);

    m_loadingScreenRequired = m_game->requiresLoadingScreen();

    const float loadingScreenRate = 5.0f;
    if (m_loadingScreenRequired)
        m_loadingScreenVisibility = std::min<float>(1.0f, m_loadingScreenVisibility + (dt * loadingScreenRate));
    else
        m_loadingScreenVisibility = std::max<float>(0.0f, m_loadingScreenVisibility - (dt * loadingScreenRate));

    m_gameAccumulatedTime += dt;
    m_frameIndex += 1;

    auto loadingScreenState = GameLoadingScreenState::Hidden;
    if (m_loadingScreenVisibility >= 1.0f)
        loadingScreenState = GameLoadingScreenState::Visible;
    else if (m_loadingScreenVisibility > 0.0f)
        loadingScreenState = GameLoadingScreenState::Transition;

    return m_game->processUpdate(dt, loadingScreenState);
}

void Host::render(gpu::CommandWriter& cmd, const HostViewport& hostViewport)
{
    // render game stack
    GameViewport viewport;
    if (m_game->processRender(viewport))
    {
        const float defaultAspect = hostViewport.width / (float)hostViewport.height;
        const float defaultFOV = 75.0f;
        const float defaultNearPlane = 0.01f;
        const float defaultFarPlane = 4000.0f;

        CameraSetup cameraSetup;
        cameraSetup.position = viewport.cameraPlacement.position.approximate();
        cameraSetup.rotation = viewport.cameraPlacement.rotation;
        cameraSetup.aspect = viewport.cameraPlacement.customAspect ? viewport.cameraPlacement.customAspect : defaultAspect;
        cameraSetup.fov = viewport.cameraPlacement.customFov ? viewport.cameraPlacement.customFov : defaultFOV;
        cameraSetup.nearPlane = viewport.cameraPlacement.customNearPlane ? viewport.cameraPlacement.customNearPlane : defaultNearPlane;
        cameraSetup.farPlane = viewport.cameraPlacement.customFarPlane ? viewport.cameraPlacement.customFarPlane : defaultFarPlane;

        Camera camera;
        camera.setup(cameraSetup);

        rendering::FrameParams params(hostViewport.width, hostViewport.height, camera);
        params.index = m_frameIndex;
        params.camera.cameraContext = m_cameraContext;
        params.time.gameTime = m_gameAccumulatedTime;
        params.time.engineRealTime = m_startTime.timeTillNow().toSeconds();
        params.time.timeOfDay = 12.0f;
        params.time.dayNightFrac = 1.0f;

        if (viewport.world)
            viewport.world->render(params);

        rendering::FrameCompositionTarget targets;
        targets.targetColorRTV = hostViewport.backBufferColor;
        targets.targetDepthRTV = hostViewport.backBufferDepth;
        targets.targetRect = Rect(0, 0, hostViewport.width, hostViewport.height);

        if (targets.targetColorRTV->flipped())
        {
            if (m_flippedColorTarget && (m_flippedColorTarget->width() != targets.targetColorRTV->width() || m_flippedColorTarget->height() != targets.targetColorRTV->height()))
            {
                m_flippedColorTarget.reset();
                m_flippedDepthTarget.reset();
            }

            if (!m_flippedColorTarget)
            {
                gpu::ImageCreationInfo info;
                info.allowRenderTarget = true;
                info.width = targets.targetColorRTV->width();
                info.height = targets.targetColorRTV->height();
                info.format = targets.targetColorRTV->format();
                info.label = "FlippedColorOutput";
                m_flippedColorTarget = GetService<gpu::DeviceService>()->device()->createImage(info);
                m_flippedColorTargetRTV = m_flippedColorTarget->createRenderTargetView();

                info.format = targets.targetDepthRTV->format();
                m_flippedDepthTarget = GetService<gpu::DeviceService>()->device()->createImage(info);
                m_flippedDepthTargetRTV = m_flippedDepthTarget->createRenderTargetView();
            }

            rendering::FrameCompositionTarget flippedTarget;
            flippedTarget.targetColorRTV = m_flippedColorTargetRTV;
            flippedTarget.targetDepthRTV = m_flippedDepthTargetRTV;
            flippedTarget.targetRect = targets.targetRect;

            if (auto sceneCmd = GetService<rendering::FrameRenderingService>()->renderFrame(params, flippedTarget))
                cmd.opAttachChildCommandBuffer(sceneCmd);

            cmd.opCopyRenderTarget(m_flippedColorTargetRTV, targets.targetColorRTV, 0, 0, true);
        }
        else
        {
            if (auto sceneCmd = GetService<rendering::FrameRenderingService>()->renderFrame(params, targets))
                cmd.opAttachChildCommandBuffer(sceneCmd);
        }
    }

    // render debug overlay
    renderOverlay(cmd, hostViewport);
}

bool Host::input(const input::BaseEvent& evt)
{
    if (processDebugInput(evt))
        return true;

    if (m_game->processInput(evt))
        return true;

    return false;
}

bool Host::shouldCaptureInput() const
{
    return !DebugPagesVisible() && !m_paused;
}

void Host::renderOverlay(gpu::CommandWriter& cmd, const HostViewport& viewport)
{
    gpu::FrameBuffer fb;
    fb.color[0].view(viewport.backBufferColor); // no clear
    fb.depth.view(viewport.backBufferDepth).clearDepth().clearStencil();
    cmd.opBeingPass(fb);

    {
        canvas::Canvas canvas(viewport.width, viewport.height);

        renderCanvas(canvas);

        if (DebugPagesVisible())
        {
            m_imgui->beginFrame(canvas, 0.01f);
            DebugPagesRender();
            m_imgui->endFrame(canvas);
        }

        GetService<canvas::CanvasService>()->render(cmd, canvas);
    }

    cmd.opEndPass();
}

void Host::renderCanvas(canvas::Canvas& canvas)
{
    m_game->processRenderCanvas(canvas);
}

bool Host::processDebugInput(const input::BaseEvent& evt)
{
    // is this a key event ?
    if (auto keyEvent = evt.toKeyEvent())
    {
        // did we press the key ?
        if (keyEvent->pressed())
        {
            switch (keyEvent->keyCode())
            {
                // Exit application when "ESC" is pressed
                case input::KeyCode::KEY_F10:
                    platform::GetLaunchPlatform().requestExit("Instant exit key pressed");
                    return true;

                // Toggle game pause
                case input::KeyCode::KEY_PAUSE:
                    m_paused = !m_paused;
                    return true;

                // Toggle debug panels
                case input::KeyCode::KEY_F1:
                    DebugPagesVisibility(!DebugPagesVisible());
                    return true;
            }
        }
    }

    // pass to game or debug panels
    if (DebugPagesVisible())
    {
        if (m_imgui->processInput(evt))
            return true;
    }

    // allow input to pass to game
    return false;
}

//--
    
END_BOOMER_NAMESPACE()
