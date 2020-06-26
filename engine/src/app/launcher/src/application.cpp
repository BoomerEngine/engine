/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "application.h"

#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingDeviceObject.h"
#include "rendering/driver/include/renderingDeviceService.h"
#include "rendering/driver/include/renderingOutput.h"
#include "base/app/include/configProperty.h"
#include "base/app/include/launcherPlatform.h"
#include "base/input/include/inputContext.h"
#include "game/host/include/gameScreen.h"
#include "game/host/include/gameHost.h"
#include "game/host/include/gameScreen_MeshViewer.h"
#include "rendering/driver/include/renderingCommandWriter.h"
#include "rendering/mesh/include/renderingMesh.h"

namespace launcher
{
    //--

    base::ConfigProperty<base::StringBuf> cvInitialGameScreen("Game.Startup", "StartupGameScreen", "game::Screen_Splash");

    //--

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
        if (m_renderingOutput)
        {
            // rendering output must always be explicitly closed
            base::GetService<DeviceService>()->device()->releaseObject(m_renderingOutput);
            m_renderingOutput = rendering::ObjectID();
        }
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
        rendering::DriverOutputInitInfo setup;
        setup.m_width = 1920;
        setup.m_height = 1080;
        setup.m_windowMaximized = false;
        setup.m_windowTitle = "BoomerEngine - Game Launcher"; // TODO: allow game to override this
        setup.m_windowInputContextGameMode = true;
        setup.m_windowShowOnTaskBar = true;
        setup.m_windowCreateInputContext = true;
        setup.m_class = rendering::DriverOutputClass::NativeWindow; // render to native window on given OS
        setup.m_class = rendering::DriverOutputClass::Fullscreen;

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

        // request input capture for game input
        auto inputContext = m_renderingWindow->windowGetInputContext();
        inputContext->requestCapture(2);

        return true;
    }

    bool LauncherApp::createGame(const base::app::CommandLine& commandline)
    {
        // TODO: allow game screen class override from commandline
        //gameClassName

        // find the root state
        const auto gameClassName = base::StringID(cvInitialGameScreen.get());
        const auto gameStateClass = RTTI::GetInstance().findClass(gameClassName);
        if (!gameStateClass)
        {
            TRACE_ERROR("Missing initial game state '{}', nothing to run", gameClassName);
            return false;
        }

        // create initial game state
        const auto gameState = gameStateClass.create<game::IScreen>();

        // create second screens
        if (commandline.hasParam("meshPath"))
        {
            const auto meshPath = base::res::ResourcePath(commandline.singleValue("meshPath"));
            const auto meshState = base::CreateSharedPtr<game::Screen_MeshViewer>(meshPath);
            gameState->startTransition(game::ScreenTransitionRequest::Replace(meshState));
        }

               
        // create the game host with the initial state
        m_gameHost = base::CreateSharedPtr<game::Host>(game::HostType::Standalone, gameState);
        return true;
    }

    bool LauncherApp::processInput(const base::input::BaseEvent& evt)
    {
        if (auto key = evt.toKeyEvent())
        {
            if (key->pressed() && key->keyCode() == base::input::KeyCode::KEY_F10)
            {
                base::platform::GetLaunchPlatform().requestExit("User fast exit");
                return true;
            }
        }

        if (m_gameHost->input(evt))
            return true;

        return false;
    }

    void LauncherApp::updateWindow()
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

        base::Rect viewport;
        rendering::ImageView colorBackBuffer, depthBackBuffer;
        if (cmd.opAcquireOutput(m_renderingOutput, viewport, colorBackBuffer, &depthBackBuffer))
        {
            game::HostViewport gameViewport;
            gameViewport.width = viewport.width();
            gameViewport.height = viewport.height();
            gameViewport.backBufferColor = colorBackBuffer;
            gameViewport.backBufferDepth = depthBackBuffer;

            renderGame(cmd, gameViewport); // game itself
            renderOverlay(cmd, gameViewport); // debug panels

            cmd.opSwapOutput(m_renderingOutput);
        }

        // submit new work to rendering device
        base::GetService<DeviceService>()->device()->submitWork(cmd.release());
    }

    //--

} // launcher

base::app::IApplication& GetApplicationInstance()
{
    static launcher::LauncherApp theApp;
    return theApp;
}