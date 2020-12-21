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

		auto dev = base::GetService<DeviceService>()->device();
		m_storage = base::RefNew<rendering::canvas::CanvasStorage>(dev);

		m_imguiHelper = new ImGui::ImGUICanvasHelper(m_storage);

        m_game = RefNew<Game>();
        return true;
    }

    void GameApp::cleanup()
    {
        m_game.reset();

		delete m_imguiHelper;
		m_imguiHelper = nullptr;

        m_renderingOutput.reset();
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
        rendering::OutputInitInfo setup;
        setup.m_width = 1920;
        setup.m_height = 1080;
        setup.m_windowMaximized = false;
        setup.m_windowTitle = "BoomerEngine - Game2D";
        setup.m_class = rendering::OutputClass::Window; // render to native window on given OS
        setup.m_windowCreateInputContext = true;
        setup.m_windowInputContextGameMode = true; // RawInput

        // create rendering output
        m_renderingOutput = renderingService->device()->createOutput(setup);
        if (!m_renderingOutput || !m_renderingOutput->window())
        {
            TRACE_ERROR("Failed to acquire window factory, no window can be created");
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
        if (auto input = m_renderingOutput->window()->windowGetInputContext())
            input->requestCapture(shouldCaptureInput() ? 2 : 0);
    }

    void GameApp::updateWindow()
    {
        // if the window wants to be closed allow it and close our app as well
        if (m_renderingOutput->window()->windowHasCloseRequest())
            base::platform::GetLaunchPlatform().requestExit("Main window closed");

        // process input from the window
        if (auto inputContext = m_renderingOutput->window()->windowGetInputContext())
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
            return m_imguiHelper->processInput(evt);
        else
            return m_game->handleInput(evt);
    }
       
    void GameApp::renderNonGameOverlay(CommandWriter& cmd, const GameViewport& vp)
    {
        if (DebugPagesVisible())
        {
			rendering::canvas::CanvasRenderer::Setup setup;
			setup.width = vp.width;
			setup.height = vp.height;
			setup.backBufferColorRTV = vp.colorTarget;
			setup.backBufferDepthRTV = vp.depthTarget;
			setup.backBufferLayout = vp.passLayout;
			setup.pixelScale = 1.0f;

			// render test to canvas
			{
				rendering::canvas::CanvasRenderer canvas(setup, m_storage);

				// ImGui
				{
					m_imguiHelper->beginFrame(canvas, 0.01f);
					DebugPagesRender();
					m_game->debug();
					m_imguiHelper->endFrame(canvas);
				}

				cmd.opAttachChildCommandBuffer(canvas.finishRecording());
			}
        }
    }

    void GameApp::renderFrame()
    {
        CommandWriter cmd("CommandBuffer");

        if (auto output = cmd.opAcquireOutput(m_renderingOutput))
        {
			GameViewport vp;
			vp.width = output.width;
			vp.height = output.height;
			vp.colorTarget = output.color;
			vp.depthTarget = output.depth;
			vp.passLayout = output.layout;

			m_game->render(cmd, vp);

			renderNonGameOverlay(cmd, vp);

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
