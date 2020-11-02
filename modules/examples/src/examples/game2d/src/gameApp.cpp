/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: example #]
***/

#include "build.h"
#include "gameApp.h"
#include "game.h"

namespace example
{
    //---

    GameApp::GameApp()
    {
    }

    bool GameApp::initialize(const CommandLine& commandline)
    {
        if (!createWindow())
            return false;

        m_imgui = ImGui::CreateContext();
        ImGui::SetCurrentContext(m_imgui);

        m_game = CreateSharedPtr<Game>();
        return true;
    }

    void GameApp::cleanup()
    {
        m_game.reset();

        if (m_imgui)
        {
            ImGui::SetCurrentContext(nullptr);
            ImGui::DestroyContext(m_imgui);
            m_imgui = nullptr;
        }

        if (m_renderingOutput)
        {
            // rendering output must always be explicitly closed
            base::GetService<DeviceService>()->device()->releaseObject(m_renderingOutput);
            m_renderingOutput = rendering::ObjectID();
        }
    }

    bool GameApp::createWindow()
    {
        // we need the rendering service for any of stuff to work, get the current instance of the rendering service from local service container
        auto renderingService  = GetService<DeviceService>();
        if (!renderingService)
        {
            TRACE_ERROR("No rendering service running (trying to run in a windowless environment?)");
            return false;
        }

        // setup rendering output as a window
        rendering::DriverOutputInitInfo setup;
        setup.m_width = 1920;
        setup.m_height = 1080;
        setup.m_windowMaximized = false;
        setup.m_windowTitle = "BoomerEngine - Game2D";
        setup.m_class = rendering::DriverOutputClass::NativeWindow; // render to native window on given OS
        setup.m_windowCreateInputContext = true;
        setup.m_windowInputContextGameMode = true; // RawInput

        // create rendering output
        m_renderingOutput = renderingService->device()->createOutput(setup);
        if (!m_renderingOutput)
        {
            TRACE_ERROR("Failed to acquire window factory, no window can be created");
            return false;
        }

        // get window
        m_renderingWindow = renderingService->device()->queryOutputWindow(m_renderingOutput);
        if (!m_renderingWindow)
        {
            TRACE_ERROR("Rendering output has no valid window");
            return false;
        }

        return true;
    }

    bool GameApp::shouldCaptureInput()
    {
        return !DebugPagesVisible();
    }

    void GameApp::update()
    {
        // measure and accumulate the dt from last frame
        double dt = std::clamp(m_lastTickTime.timeTillNow().toSeconds(), 0.0001, 0.1);
        m_lastTickTime.resetToNow();

        // update window stuff - read and process input
        updateWindow();

        // update the game
        m_game->tick(dt);

        // record frame content and send it for rendering
        renderFrame();

        // capture input to the window if required
        if (m_renderingWindow && m_renderingWindow->windowGetInputContext())
            m_renderingWindow->windowGetInputContext()->requestCapture(shouldCaptureInput() ? 2 : 0);
    }

    void GameApp::updateWindow()
    {
        // if the window wants to be closed allow it and close our app as well
        if (m_renderingWindow->windowHasCloseRequest())
            base::platform::GetLaunchPlatform().requestExit("Main window closed");

        // process input from the window
        if (auto inputContext = m_renderingWindow->windowGetInputContext())
        {
            while (auto inputEvent = inputContext->pull())
                processInput(*inputEvent);
        }
    }

    bool GameApp::processInput(const BaseEvent& evt)
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
                case KeyCode::KEY_ESCAPE:
                    platform::GetLaunchPlatform().requestExit("ESC key pressed");
                    return true;

                    // Toggle game pause
                case KeyCode::KEY_PAUSE:
                    m_paused = !m_paused;
                    return true;

                    // Toggle debug panels
                case KeyCode::KEY_F1:
                    DebugPagesVisibility(!DebugPagesVisible());
                    return true;
                }
            }
        }

        // pass to game or debug panels
        if (DebugPagesVisible())
            return ImGui::ProcessInputEvent(m_imgui, evt);
        else
            return m_game->handleInput(evt);
    }
       
    void GameApp::renderNonGameOverlay(CommandWriter& cmd, uint32_t width, uint32_t height, const rendering::ImageView& colorTarget, const rendering::ImageView& depthTarget)
    {
        FrameBuffer fb;
        fb.color[0].view(colorTarget); // no clear
        fb.depth.view(depthTarget).clearDepth().clearStencil();
        cmd.opBeingPass(fb);

        if (DebugPagesVisible())
        {
            base::canvas::Canvas canvas(width, height);

            {
                ImGui::BeginCanvasFrame(canvas);
                DebugPagesRender();
                m_game->debug();
                ImGui::EndCanvasFrame(canvas);
            }

            CanvasRenderingParams renderingParams;
            renderingParams.frameBufferWidth = width;
            renderingParams.frameBufferHeight = height;
            base::GetService<CanvasService>()->render(cmd, canvas, renderingParams);
        }

        cmd.opEndPass();
    }

    void GameApp::renderFrame()
    {
        // create a command buffer writer and tell the "scene" to render to it
        CommandWriter cmd("CommandBuffer");

        base::Rect viewport;
        rendering::ImageView colorBackBuffer, depthBackBuffer;
        if (cmd.opAcquireOutput(m_renderingOutput, viewport, colorBackBuffer, &depthBackBuffer))
        {
            m_game->render(cmd, viewport.width(), viewport.height(), colorBackBuffer, depthBackBuffer);

            renderNonGameOverlay(cmd, viewport.width(), viewport.height(), colorBackBuffer, depthBackBuffer);

            cmd.opSwapOutput(m_renderingOutput);
        }

        // submit new work to rendering device
        base::GetService<DeviceService>()->device()->submitWork(cmd.release());
    }

} // example

base::app::IApplication& GetApplicationInstance()
{
    static example::GameApp theApp;
    return theApp;
}