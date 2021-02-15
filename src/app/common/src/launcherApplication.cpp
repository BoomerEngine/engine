/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "launcherApplication.h"

#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingDeviceService.h"
#include "rendering/device/include/renderingOutput.h"
#include "base/app/include/configProperty.h"
#include "base/app/include/launcherPlatform.h"
#include "base/input/include/inputContext.h"
#include "game/host/include/gameHost.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/mesh/include/renderingMesh.h"
#include "game/host/include/game.h"

namespace application
{
    //--

    base::ConfigProperty<base::StringBuf> cvGameStartupGamePath("Game", "StartupGame", "");

    //--

    LauncherApp::LauncherApp(base::StringView title)
        : m_title(title)
    {}
    
    bool LauncherApp::initialize(const base::app::CommandLine& commandline)
    {
        if (!createGame(commandline))
            return false;

        if (!createWindow(commandline))
            return false;

        return true;
    }

    void LauncherApp::cleanup()
    {
        // destroy game
        m_gameHost.reset();

        // close rendering window
        m_renderingOutput.reset();
    }

    void LauncherApp::update()
    {
        PC_SCOPE_LVL0(AppUpdate);

        // measure and accumulate the dt from last frame
        const auto timeDelta = m_lastTickTime.timeTillNow().toSeconds();
        const double dt = std::clamp<double>(timeDelta, 0.0, 0.1);
        m_lastTickTime.resetToNow();

        // update window stuff - read and process input
        // NOTE: the processed input may activate up the game transitions
        updateWindow();

        // update game host state stack
        updateGame(dt);

        // render the frame
        renderFrame();
    }

    //--

    bool LauncherApp::createWindow(const base::app::CommandLine& commandline)
    {
        // we need the rendering service for any of stuff to work, get the current instance of the rendering service from local service container
        auto renderingService = base::GetService<DeviceService>();
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
        setup.m_windowTitle = m_title ? m_title : "BoomerEngine - Game Launcher"; // TODO: allow game to override this
        setup.m_windowInputContextGameMode = true;
        setup.m_windowShowOnTaskBar = true;
        setup.m_windowCreateInputContext = true;
        setup.m_class = rendering::OutputClass::Window; // render to native window on given OS
        
        // switch fullscreen/windowed
		if (commandline.hasParam("fullscreen"))
			setup.m_windowStartInFullScreen = true;
        else if (commandline.hasParam("window") || commandline.hasParam("windowed"))
			setup.m_windowStartInFullScreen = false;

        // resolution
        if (commandline.hasParam("width") && commandline.hasParam("height"))
        {
            int newWidth = commandline.singleValueInt("width", -1);
            int newHeight = commandline.singleValueInt("height", -1);
            if (newWidth != -1 && newHeight != -1)
            {
                setup.m_width = newWidth;
                setup.m_height = newHeight;
            }
        }

        // create rendering output
        m_renderingOutput = renderingService->device()->createOutput(setup);
        if (!m_renderingOutput || !m_renderingOutput->window())
        {
            TRACE_ERROR("Failed to acquire window factory, no window can be created");
            return false;
        }

        // request input capture for game input
        auto inputContext = m_renderingOutput->window()->windowGetInputContext();
        inputContext->requestCapture(2);

        return true;
    }

    bool ParseGameInitData(const base::app::CommandLine& commandline, game::GameInitData& outData)
    {
        // TODO: initial spawn position,rotation + initial world
        return true;
    }

    bool LauncherApp::createGame(const base::app::CommandLine& commandline)
    {
        // fill in the game init info
        game::GameInitData initData;
        if (!ParseGameInitData(commandline, initData))
            return false;

        // create the game host with the created game
        const auto game = base::RefNew<game::Game>();
        m_gameHost = base::RefNew<game::Host>(game::HostType::Standalone, game);
        return true;
    }

    bool LauncherApp::processInput(const base::input::BaseEvent& evt)
    {
        if (m_gameHost->input(evt))
            return true;

        return false;
    }

    void LauncherApp::updateWindow()
    {
        // if the window wants to be closed allow it and close our app as well
        if (m_renderingOutput->window()->windowHasCloseRequest())
            base::platform::GetLaunchPlatform().requestExit("Main window closed");

        // process input from the window
        if (auto inputContext = m_renderingOutput->window()->windowGetInputContext())
        {
            // process all events as they come
            while (auto inputEvent = inputContext->pull())
                processInput(*inputEvent);

            // capture input to the window if required
            inputContext->requestCapture(m_gameHost->shouldCaptureInput() ? 2 : 0);
        }
    }

    void LauncherApp::updateGame(double dt)
    {
        // update the game state stack
        if (!m_gameHost->update(dt))
            base::platform::GetLaunchPlatform().requestExit("Game stack empty");
    }

    //--

    void LauncherApp::renderGame(rendering::command::CommandWriter& parentCmd, const game::HostViewport& viewport)
    {
        rendering::command::CommandWriter cmd(parentCmd.opCreateChildCommandBuffer());
        m_gameHost->render(cmd, viewport);
    }

    void LauncherApp::renderOverlay(rendering::command::CommandWriter& cmd, const game::HostViewport& viewport)
    {
        // TODO: imgui debug panels 
    }

    void LauncherApp::renderFrame()
    {
        rendering::command::CommandWriter cmd("AppFrame");

        if (auto output = cmd.opAcquireOutput(m_renderingOutput))
        {
            game::HostViewport gameViewport;
            gameViewport.width = output.width;
            gameViewport.height = output.height;
            gameViewport.backBufferColor = output.color;
            gameViewport.backBufferDepth = output.depth;

            renderGame(cmd, gameViewport); // game itself
            renderOverlay(cmd, gameViewport); // debug panels

            cmd.opSwapOutput(m_renderingOutput);
        }

        // submit new work to rendering device
        base::GetService<DeviceService>()->device()->submitWork(cmd.release());
    }

    //--

} // application

