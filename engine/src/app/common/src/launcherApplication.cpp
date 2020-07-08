/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "launcherApplication.h"

#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingDeviceObject.h"
#include "rendering/driver/include/renderingDeviceService.h"
#include "rendering/driver/include/renderingOutput.h"
#include "base/app/include/configProperty.h"
#include "base/app/include/launcherPlatform.h"
#include "base/input/include/inputContext.h"
#include "game/host/include/gameScreen.h"
#include "game/host/include/gameHost.h"
#include "game/host/include/gameFactory.h"
#include "rendering/driver/include/renderingCommandWriter.h"
#include "rendering/mesh/include/renderingMesh.h"

namespace application
{
    //--

    base::ConfigProperty<base::StringBuf> cvGameFactoryClass("Game", "FactoryClass", "");

    //--

    LauncherApp::LauncherApp(base::StringView<char> title)
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
        setup.m_windowTitle = m_title ? m_title : "BoomerEngine - Game Launcher"; // TODO: allow game to override this
        setup.m_windowInputContextGameMode = true;
        setup.m_windowShowOnTaskBar = true;
        setup.m_windowCreateInputContext = true;
        setup.m_class = rendering::DriverOutputClass::NativeWindow; // render to native window on given OS
        

        // switch fullscreen/windowed
        if (commandline.hasParam("fullscreen"))
            setup.m_class = rendering::DriverOutputClass::Fullscreen;
        else if (commandline.hasParam("window") || commandline.hasParam("windowed"))
            setup.m_class = rendering::DriverOutputClass::NativeWindow;

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


    base::SpecificClassType<game::IGameFactory> GetGameFactoryClass(const base::app::CommandLine& commandline)
    {
        // use game from commandline
        if (commandline.hasParam("game"))
        {
            const auto gameClassName = commandline.singleValue("game");
            TRACE_INFO("Using game factory class '{}' from commandline", gameClassName);

            const auto gameClass = RTTI::GetInstance().findClass(base::StringID(gameClassName)).cast<game::IGameFactory>();
            if (!gameClass)
            {
                TRACE_ERROR("Game factory class '{}' not found", gameClassName);
                return nullptr;
            }

            return gameClass;
        }

        // use game class from config
        if (const auto gameClassName = cvGameFactoryClass.get())
        {
            TRACE_INFO("Using game factory class '{}' from config", gameClassName);

            const auto gameClass = RTTI::GetInstance().findClass(base::StringID(gameClassName)).cast<game::IGameFactory>();
            if (!gameClass)
            {
                TRACE_ERROR("Game factory class '{}' not found", gameClassName);
                return nullptr;
            }

            return gameClass;
        }

        // enumerate game classes
        base::InplaceArray<base::SpecificClassType<game::IGameFactory>, 5> gameFactoryClasses;
        RTTI::GetInstance().enumClasses(gameFactoryClasses);
        TRACE_INFO("Found {} game factory classes", gameFactoryClasses.size());
        if (gameFactoryClasses.empty())
        {
            TRACE_ERROR("No game factories linked with executable, unable to start any game");
            return nullptr;
        }

        // TODO: choice using debug menu ?

        // use just first game class
        TRACE_INFO("Using game factory '{}'", gameFactoryClasses[0]->name());
        return gameFactoryClasses[0];
    }

    bool ParseGameInitData(const base::app::CommandLine& commandline, game::GameInitData& outData)
    {
        // TODO: initial spawn position,rotation + initial world
        return true;
    }

    bool LauncherApp::createGame(const base::app::CommandLine& commandline)
    {
        // get game factory
        const auto gameFactoryClass = GetGameFactoryClass(commandline);
        if (!gameFactoryClass)
            return false;

        // fill in the game init info
        game::GameInitData initData;
        if (!ParseGameInitData(commandline, initData))
            return false;

        // create the game factory
        const auto gameFactory = gameFactoryClass.create();
        const auto game = gameFactory->createGame(initData);
        if (!game)
            return false;

        // create the game host with the created game
        m_gameHost = base::CreateSharedPtr<game::Host>(game::HostType::Standalone, game);
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
        if (m_renderingWindow->windowHasCloseRequest())
            base::platform::GetLaunchPlatform().requestExit("Main window closed");

        // process input from the window
        if (auto inputContext = m_renderingWindow->windowGetInputContext())
        {
            while (auto inputEvent = inputContext->pull())
                processInput(*inputEvent);
        }

        // capture input to the window if required
        if (m_renderingWindow && m_renderingWindow->windowGetInputContext())
            m_renderingWindow->windowGetInputContext()->requestCapture(m_gameHost->shouldCaptureInput() ? 2 : 0);
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

} // application

