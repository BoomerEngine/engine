/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "screen.h"
#include "screenDebugMenu.h"
#include "screenStack.h"

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

RTTI_BEGIN_TYPE_NATIVE_CLASS(GameScreenStack);
RTTI_END_TYPE();

GameScreenStack::GameScreenStack(IGamePlatform* platform)
    : m_platform(AddRef(platform))
{
    m_imgui = new ImGui::ImGUICanvasHelper();
}

GameScreenStack::~GameScreenStack()
{
    delete m_imgui;
    m_imgui = nullptr;
}

void GameScreenStack::requestExit()
{
    if (!m_exitRequested)
    {
        m_exitRequested = true;
        TRACE_INFO("IGame host exit requested");
    }
}
 
void GameScreenStack::pushTransition(IGameScreen* screen, GameTransitionMode mode, bool instant /*= false*/)
{
    PendingTransition info;
    info.mode = mode;
    info.screen = AddRef(screen);
    info.instant = instant;
    m_pendingTransition.push(info);
}

void GameScreenStack::attachScreen(IGameScreen* screen, float fadeInTime)
{
    DEBUG_CHECK_RETURN(screen);
    DEBUG_CHECK_RETURN(screen->m_host == nullptr);
    
    auto& info = m_screens.emplaceBack();
    info.screen = AddRef(screen);

    if (fadeInTime > 0.0f)
    {
        info.fadeSpeed = 1.0f / fadeInTime;
        info.fadeVisibility = 0.0f;
    }
    else
    {
        info.fadeSpeed = 0.0f;
        info.fadeVisibility = 1.0f;
    }

    screen->m_host = this;

    screen->handleAttached();
}

void GameScreenStack::detachScreen(IGameScreen* screen)
{
    DEBUG_CHECK_RETURN(screen);
    DEBUG_CHECK_RETURN(screen->m_host == this);

    for (auto i : m_screens.indexRange())
    {
        if (m_screens[i].screen == screen)
        {
            screen->m_host = nullptr;
            screen->handleDetached();

            m_screens.erase(i); // NOTE: this may free memory
        }
    }
}

bool GameScreenStack::update(double dt)
{ 
    PC_SCOPE_LVL0(GameHostUpdate);

    if (m_exitRequested)
    {
        TRACE_INFO("IGame host exiting as exit was requested");
        return false;
    }

    updateTransitions(dt);

    if (m_screens.empty() && !hasPendingTransition())
    {
        TRACE_INFO("IGame host exiting as there are active game controllers");
        return false;
    }

    for (const auto& screen : m_screens)
        screen.screen->handleUpdate(dt);

    return true;
}

bool GameScreenStack::hasTransitionInProgress() const
{
    for (const auto& screen : m_screens)
        if (screen.fadeSpeed != 0.0f)
            return true;

    return false;
}

bool GameScreenStack::updateCurrentScreenFades(float dt)
{
    // checkout the top screen only
    if (!m_screens.empty())
    {
        auto& top = m_screens.back();

        // fade in
        if (top.fadeSpeed > 0.0f)
        {
            top.fadeVisibility += top.fadeSpeed * dt;
            if (top.fadeVisibility < 1.0f)
                return true; // still fading in

            top.fadeVisibility = 1.0f;
            top.fadeSpeed = 0.0f;
        }

        // fade out
        if (top.fadeSpeed < 0.0f)
        {
            top.fadeVisibility += top.fadeSpeed * dt;
            if (top.fadeVisibility > 0.0f)
                return true; // still fading out

            top.fadeVisibility = 0.0f;
            top.fadeSpeed = 0.0f;

            detachScreen(top.screen);
        }
    }

    // not in fade out/fade in
    return false;
}

bool GameScreenStack::updatePendingFadeOut()
{
    while (!m_pendingFadeOutScreens.empty())
    {
        auto screen = m_pendingFadeOutScreens.top();
        m_pendingFadeOutScreens.pop();

        auto fadeOutTime = screen.instant ? 0.0f : std::max<float>(0.0f, screen.screen->queryFadeOutTime());
        if (fadeOutTime > 0.0f)
        {
            for (auto& info : m_screens)
            {
                if (info.screen == screen.screen)
                {
                    info.fadeSpeed = -1.0f / fadeOutTime;
                    return true;
                }
            }
        }
        else
        {
            detachScreen(screen.screen);
        }
    }

    return false;
}

bool GameScreenStack::updatePendingFadeIn()
{
    while (!m_pendingFadeInScreens.empty())
    {
        // get first screen to add
        auto screen = m_pendingFadeInScreens.top();
        m_pendingFadeInScreens.pop();

        // initialize screen
        auto fadeInTime = screen.instant ? 0.0f : std::max<float>(0.0f, screen.screen->queryFadeInTime());
        attachScreen(screen.screen, fadeInTime);

        // if not using the instant transition give the screen time to fade int
        if (fadeInTime > 0.0f)
            return true;
    }

    return false;
}

bool GameScreenStack::updatePendingTransition()
{
    if (m_pendingTransition.empty())
        return false;

    auto transition = m_pendingTransition.top();
    m_pendingTransition.pop();

    if (transition.mode == GameTransitionMode::ReplaceAll)
    {
        for (auto index : m_screens.indexRange().reversed())
        {
            PendingScreen info;
            info.screen = m_screens[index].screen;
            info.instant = transition.instant;
            m_pendingFadeOutScreens.push(info);
        }
    }
    else if (transition.mode == GameTransitionMode::ReplaceTop)
    {
        if (!m_screens.empty())
        {
            PendingScreen info;
            info.screen = m_screens.back().screen;
            info.instant = transition.instant;
            m_pendingFadeOutScreens.push(info);
        }
    }

    if (transition.screen)
    {
        PendingScreen info;
        info.screen = transition.screen;
        info.instant = transition.instant;
        m_pendingFadeInScreens.push(info);
    }

    return true;
}

void GameScreenStack::updateTransitions(float dt)
{
    // update screens and if there's nothing fading at the moment then process the pending screen transitions
    if (!updateCurrentScreenFades(dt))
    {
        do
        {
            if (updatePendingFadeOut())
                break; // something started to fade out

            if (updatePendingFadeIn())
                break; // something started to fade in

        }
        while (updatePendingTransition());
    }
}

void GameScreenStack::render(gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output)
{
    // render game controller
    {
        auto index = m_screens.lastValidIndex();
        while (index > 0)
        {
            const auto& info = m_screens[index];
            if (info.screen->queryOpaqueState())
                break;
            index -= 1;
        }

        while (index >= 0 && index <= m_screens.lastValidIndex())
        {
            const auto& info = m_screens[index];
            info.screen->handleRender(cmd, output, info.fadeVisibility);
            
            ++index;
        }
    }

    // render debug UI
    renderImGuiDebugOverlay(cmd, output);
}

void GameScreenStack::renderImGuiDebugOverlay(gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output)
{
#ifndef BUILD_FINAL
    if (DebugPagesVisible())
    {
        gpu::FrameBuffer fb;
        fb.color[0].view(output.color); // no clear
        fb.depth.view(output.depth).clearDepth().clearStencil();
        cmd.opBeingPass(fb);

        {
            Canvas canvas(output.width, output.height);

            m_imgui->beginFrame(canvas, 0.01f);
            DebugPagesRender();

            {
                auto index = m_screens.lastValidIndex();
                while (index >= 0)
                {
                    const auto& info = m_screens[index];
                    info.screen->handleRenderImGuiDebugOverlay();
                    index -= 1;
                }
            }

            m_imgui->endFrame(canvas);

            GetService<CanvasService>()->render(cmd, canvas);
        }

        cmd.opEndPass();
    }
#endif
}

bool GameScreenStack::input(const InputEvent& evt)
{
    // always send input to debug code first
    if (processDebugInput(evt))
        return true;

    // ignore normal input during transitions
    if (hasTransitionInProgress())
        return false;

    // render game controller
    for (auto i : m_screens.indexRange().reversed())
    {
        const auto& info = m_screens[i];

        if (info.screen->handleInput(evt))
            return true;

        if (info.screen->queryFilterAllInput())
            break;
    }

    return false;
}

bool GameScreenStack::shouldCaptureInput() const
{
#ifndef BUILD_FINAL
    if (DebugPagesVisible())
        return false;
#endif

    for (auto i : m_screens.indexRange().reversed())
    {
        const auto& info = m_screens[i];

        if (info.screen->queryInputCaptureState())
            return true;

        if (info.screen->queryFilterAllInput())
            break;
    }

    return false;
}

bool GameScreenStack::processDebugInput(const InputEvent& evt)
{
#ifndef BUILD_FINAL
    if (auto keyEvent = evt.toKeyEvent())
    {
        if (keyEvent->pressed())
        {
            switch (keyEvent->keyCode())
            {
                // Fast exit key that bypasses all menus
                case InputKey::KEY_F10:
                    requestExit();
                    return true;

                // Toggle input passthrough
                case InputKey::KEY_F11:
                    m_imguiOverlayInputPassThrough = !m_imguiOverlayInputPassThrough;
                    return true;

                // Toggle debug panels
                case InputKey::KEY_F1:
                    DebugPagesVisibility(!DebugPagesVisible());
                    return true;
            }
        }
    }

    // pass to game or debug panels
    if (DebugPagesVisible())
    {
        const auto consumed = m_imgui->processInput(evt);
        return consumed || !m_imguiOverlayInputPassThrough;
    }
#endif

    // allow input to pass to game
    return false;
}

//--
    
END_BOOMER_NAMESPACE()
