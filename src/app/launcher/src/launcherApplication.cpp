/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "launcherApplication.h"

#include "core/app/include/configProperty.h"
#include "core/app/include/launcherPlatform.h"
#include "core/input/include/inputContext.h"
#include "core/containers/include/path.h"

#include "gpu/device/include/device.h"
#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/output.h"
#include "gpu/device/include/commandWriter.h"

#include "engine/game/include/game.h"
#include "engine/game/include/gameHost.h"

BEGIN_BOOMER_NAMESPACE()

//--

ConfigProperty<StringBuf> cvGameStartupGamePath("Game", "StartupGame", "");

//--

LauncherApp::LauncherApp(StringView title)
    : m_title(title)
{}
    
bool LauncherApp::initialize(const app::CommandLine& commandline)
{
    if (!createWindow(commandline))
        return false;

    if (!createGame(commandline))
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

bool LauncherApp::createWindow(const app::CommandLine& commandline)
{
    // we need the rendering service for any of stuff to work, get the current instance of the rendering service from local service container
    auto renderingService = GetService<DeviceService>();
    if (!renderingService)
    {
        TRACE_ERROR("No rendering service running (trying to run in a windowless environment?)");
        return false;
    }

    // setup rendering output as a window
    gpu::OutputInitInfo setup;
    setup.m_width = 1920;
    setup.m_height = 1080;
    setup.m_windowMaximized = false;
    setup.m_windowTitle = m_title ? m_title : "BoomerEngine - Game Launcher"; // TODO: allow game to override this
    //setup.m_windowInputContextGameMode = true;
    setup.m_windowShowOnTaskBar = true;
    setup.m_windowCreateInputContext = true;
    setup.m_windowInputContextGameMode = true;
    setup.m_class = gpu::OutputClass::Window; // render to native window on given OS
        
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
    return true;
}

bool ParseGameStartInfo(const app::CommandLine& commandline, GameStartInfo& outData)
{
    bool validInfo = false;

    const auto& startuScenePath = commandline.singleValue("cookedScenePath");
    if (ValidateDepotFilePath(startuScenePath))
    {
        outData.scenePath = startuScenePath;
        TRACE_INFO("Using override startup scene: '{}'", outData.scenePath);
        validInfo = true;
    }

    // TODO: initial spawn position,rotation

    return validInfo;
}

bool LauncherApp::createGame(const app::CommandLine& commandline)
{
    // create the game host with the created game
    const auto game = RefNew<Game>();
    m_gameHost = RefNew<Host>(HostType::Standalone, game);

    // schedule first transition using commandline arguments
    GameStartInfo initData;
    if (ParseGameStartInfo(commandline, initData))
    {
        game->scheduleNewGameWorldTransition(initData);
    }
    else
    {
        // TODO: load project's startup world
    }

    return true;
}

bool LauncherApp::processInput(const input::BaseEvent& evt)
{
    if (m_gameHost->input(evt))
        return true;

    return false;
}

void LauncherApp::updateWindow()
{
    // if the window wants to be closed allow it and close our app as well
    if (m_renderingOutput->window()->windowHasCloseRequest())
        platform::GetLaunchPlatform().requestExit("Main window closed");

    // process input from the window
    if (auto inputContext = m_renderingOutput->window()->windowGetInputContext())
    {
        // process all events as they come
        while (auto inputEvent = inputContext->pull())
            processInput(*inputEvent);

        // capture input to the window if required
        inputContext->requestCapture(m_gameHost->shouldCaptureInput() ? 1 : 0);
    }
}

void LauncherApp::updateGame(double dt)
{
    // update the game state stack
    if (!m_gameHost->update(dt))
        platform::GetLaunchPlatform().requestExit("Game stack empty");
}

//--

void LauncherApp::renderGame(gpu::CommandWriter& parentCmd, const HostViewport& viewport)
{
    gpu::CommandWriter cmd(parentCmd.opCreateChildCommandBuffer());
    m_gameHost->render(cmd, viewport);
}

void LauncherApp::renderOverlay(gpu::CommandWriter& cmd, const HostViewport& viewport)
{
    // TODO: imgui debug panels 
}

void LauncherApp::renderFrame()
{
    gpu::CommandWriter cmd("AppFrame");

    if (auto output = cmd.opAcquireOutput(m_renderingOutput))
    {
        HostViewport gameViewport;
        gameViewport.width = output.width;
        gameViewport.height = output.height;
        gameViewport.backBufferColor = output.color;
        gameViewport.backBufferDepth = output.depth;

        renderGame(cmd, gameViewport); // game itself
        renderOverlay(cmd, gameViewport); // debug panels

        cmd.opSwapOutput(m_renderingOutput);
    }

    // submit new work to rendering device
    GetService<DeviceService>()->device()->submitWork(cmd.release());
}

//--

END_BOOMER_NAMESPACE()

