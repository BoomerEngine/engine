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

#include "gpu/device/include/framebuffer.h"
#include "gpu/device/include/commandWriter.h"
#include "engine/canvas/include/service.h"
#include "engine/rendering/include/params.h"
#include "engine/rendering/include/service.h"
#include "engine/rendering/include/cameraContext.h"
#include "gpu/device/include/image.h"
#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/device.h"

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
    if (hostViewport.backBufferColor->flipped())
    {
        if (m_flippedColorTarget && (m_flippedColorTarget->width() != hostViewport.backBufferColor->width() || m_flippedColorTarget->height() != hostViewport.backBufferColor->height()))
        {
            m_flippedColorTarget.reset();
            m_flippedDepthTarget.reset();
        }

        if (!m_flippedColorTarget)
        {
            gpu::ImageCreationInfo info;
            info.allowRenderTarget = true;
            info.width = hostViewport.backBufferColor->width();
            info.height = hostViewport.backBufferColor->height();
            info.format = hostViewport.backBufferColor->format();
            info.label = "FlippedColorOutput";
            m_flippedColorTarget = GetService<gpu::DeviceService>()->device()->createImage(info);
            m_flippedColorTargetRTV = m_flippedColorTarget->createRenderTargetView();

            info.format = hostViewport.backBufferDepth->format();
            m_flippedDepthTarget = GetService<gpu::DeviceService>()->device()->createImage(info);
            m_flippedDepthTargetRTV = m_flippedDepthTarget->createRenderTargetView();
        }

        rendering::FrameCompositionTarget flippedTarget;
        flippedTarget.targetColorRTV = m_flippedColorTargetRTV;
        flippedTarget.targetDepthRTV = m_flippedDepthTargetRTV;
        flippedTarget.targetRect = Rect(0, 0, hostViewport.width, hostViewport.height);

        m_game->processRenderView(cmd, flippedTarget, m_cameraContext, &m_frameStats);

        cmd.opCopyRenderTarget(m_flippedColorTargetRTV, hostViewport.backBufferColor, 0, 0, true);
    }
    else
    {
        rendering::FrameCompositionTarget targets;
        targets.targetColorRTV = hostViewport.backBufferColor;
        targets.targetDepthRTV = hostViewport.backBufferDepth;
        targets.targetRect = Rect(0, 0, hostViewport.width, hostViewport.height);

        m_game->processRenderView(cmd, targets, m_cameraContext, &m_frameStats);
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
