/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "gameHost.h"
#include "gameScreen.h"
#include "gameEvent.h"
#include "game.h"

#include "rendering/device/include/renderingFramebuffer.h"
#include "rendering/device/include/renderingCommandWriter.h"

#include "base/app/include/launcherPlatform.h"
#include "base/canvas/include/canvas.h"
#include "base/input/include/inputStructures.h"
#include "base/imgui/include/debugPageService.h"
#include "base/imgui/include/imgui_integration.h"
#include "rendering/canvas/include/renderingCanvasService.h"

namespace game
{
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
    }

    Host::~Host()
    {
        delete m_imgui;
        m_imgui = nullptr;
    }
    
    bool Host::update(double dt)
    { 
        PC_SCOPE_LVL0(GameHostUpdate);
        return m_game->processUpdate(dt);
    }

    void Host::render(rendering::command::CommandWriter& cmd, const HostViewport& viewport)
    {
        // render game stack
        m_game->processRender(cmd, viewport);

        // render debug overlay
        renderOverlay(cmd, viewport);

    }

    bool Host::input(const base::input::BaseEvent& evt)
    {
        if (processDebugInput(evt))
            return true;

        if (m_game->processInput(evt))
            return true;

        return false;
    }

    bool Host::shouldCaptureInput() const
    {
        return !base::DebugPagesVisible() && !m_paused;
    }

    void Host::renderOverlay(rendering::command::CommandWriter& cmd, const HostViewport& viewport)
    {
        // render canvas overlay
        {
            rendering::FrameBuffer fb;
            fb.color[0].view(viewport.backBufferColor); // no clear
            fb.depth.view(viewport.backBufferDepth).clearDepth().clearStencil();
            cmd.opBeingPass(fb);

            if (base::DebugPagesVisible())
            {
                base::canvas::Canvas canvas(viewport.width, viewport.height);

                {
                    m_imgui->beginFrame(canvas, 0.01f);
                    base::DebugPagesRender();
                    m_game->handleDebug();
                    m_imgui->endFrame(canvas);
                }

                base::GetService<rendering::canvas::CanvasRenderService>()->render(cmd, canvas);
            }

            cmd.opEndPass();
        }
    }

    bool Host::processDebugInput(const base::input::BaseEvent& evt)
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
                case base::input::KeyCode::KEY_F10:
                    base::platform::GetLaunchPlatform().requestExit("Instant exit key pressed");
                    return true;

                    // Toggle game pause
                case base::input::KeyCode::KEY_PAUSE:
                    m_paused = !m_paused;
                    return true;

                    // Toggle debug panels
                case base::input::KeyCode::KEY_F1:
                    base::DebugPagesVisibility(!base::DebugPagesVisible());
                    return true;
                }
            }
        }

        // pass to game or debug panels
        if (base::DebugPagesVisible())
        {
            //ImGui::ProcessInputEvent(m_imgui, evt);
            return true;
        }

        // allow input to pass to game
        return false;
    }

    //--
    
} // game
