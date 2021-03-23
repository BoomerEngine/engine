/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "launcherApplication.h"

#include "core/app/include/configProperty.h"
#include "core/app/include/launcherPlatform.h"
#include "core/app/include/projectSettingsService.h"
#include "core/input/include/inputContext.h"
#include "core/containers/include/path.h"

#ifdef HAS_ENGINE_WORLD_COMPILER
    #include "engine/world/include/rawWorldData.h"
#endif

#include "engine/world/include/compiledWorldData.h"

#include "gpu/device/include/device.h"
#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/output.h"
#include "gpu/device/include/framebuffer.h"
#include "gpu/device/include/commandWriter.h"

#include "game/common/include/screenStack.h"
#include "game/common/include/screenSinglePlayerGame.h"
#include "game/common/include/settings.h"
#include "game/common/include/platformService.h"
#include "core/app/include/configService.h"

BEGIN_BOOMER_NAMESPACE()

//--

ConfigProperty<StringBuf> cvGameStartupGamePath("Game", "StartupGame", "");

ConfigProperty<bool> cvGameViewportFullscreen("Game.Viewport", "Fullscreen", false);
ConfigProperty<uint32_t> cvGameViewportWidth("Game.Viewport", "Width", 1920);
ConfigProperty<uint32_t> cvGameViewportHeight("Game.Viewport", "Height", 1080);

//--

LauncherApp::LauncherApp(StringView title)
    : m_title(title)
{}
    
bool LauncherApp::initialize(const CommandLine& commandline)
{
    if (!parseWindowParams(commandline))
        return false;

    if (!createWindow())
        return false;

    if (!createGame(commandline))
        return false;

    return true;
}

void LauncherApp::cleanup()
{
    // destroy game
    m_screenStack.reset();

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

bool LauncherApp::parseWindowParams(const CommandLine& commandline)
{
    // fullscreen/windowed
    if (commandline.hasParam("fullscreen"))
        cvGameViewportFullscreen.set(true);
    else if (commandline.hasParam("windowed") || commandline.hasParam("window"))
        cvGameViewportFullscreen.set(false);

    // resolution
    if (commandline.hasParam("width") && commandline.hasParam("height"))
    {
        int newWidth = commandline.singleValueInt("width", -1);
        int newHeight = commandline.singleValueInt("height", -1);
        if (newWidth != -1 && newHeight != -1)
        {
            cvGameViewportWidth.set(newWidth);
            cvGameViewportHeight.set(newHeight);
        }
    }

    return true;
}

void LauncherApp::toggleFullscreen()
{
    const auto flag = cvGameViewportFullscreen.get();
    cvGameViewportFullscreen.set(!flag);

    TRACE_INFO("Toggling fullscreen {}", cvGameViewportFullscreen.get() ? "ON" : "OFF");
    if (!createWindow())
    {
        TRACE_ERROR("Failed to toggle fullscreen!");

        cvGameViewportFullscreen.set(flag);

        if (!createWindow())
        {
            TRACE_ERROR("Window lost");
        }
    }
    else
    {
        GetService<ConfigService>()->requestSave();
    }
}

bool LauncherApp::createWindow()
{
    // we need the rendering service for any of stuff to work, get the current instance of the rendering service from local service container
    auto renderingService = GetService<DeviceService>();
    if (!renderingService)
    {
        TRACE_ERROR("No rendering service running (trying to run in a windowless environment?)");
        return false;
    }

    // close current output
    if (m_renderingOutput)
    {
        m_renderingOutput.reset();
        renderingService->device()->sync(true);
        renderingService->device()->sync(true);
    }

    // setup rendering output as a window
    gpu::OutputInitInfo setup;
    setup.m_width = cvGameViewportWidth.get();
    setup.m_height = cvGameViewportHeight.get();
    setup.m_class = gpu::OutputClass::Window;
    setup.m_windowMaximized = false;
    setup.m_windowTitle = m_title ? m_title : "BoomerEngine - Game Launcher"; // TODO: allow game to override this
    setup.m_windowShowOnTaskBar = true;
    setup.m_windowCreateInputContext = true;
    setup.m_windowInputContextGameMode = true;
    setup.m_windowStartInFullScreen = cvGameViewportFullscreen.get();

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

bool ParseGameStartInfo(const CommandLine& commandline, GameScreenSinglePlayerSetup& outData)
{
    const auto& worldPath = commandline.singleValue("worldPath");
    if (ValidateDepotFilePath(worldPath))
    {
        const auto cls = LoadClass(worldPath);
        if (cls == CompiledWorldData::GetStaticClass())
        {
            TRACE_INFO("Using compiled world: '{}'", worldPath);
            outData.worldPath = worldPath;
        }
#ifdef HAS_ENGINE_WORLD_COMPILER
        else if (cls == RawWorldData::GetStaticClass())
        {
            TRACE_INFO("Using raw world: '{}'", worldPath);
            outData.worldPath = worldPath;
        }
#endif
        else
        {
            TRACE_ERROR("Invalid startup resource: '{}'", worldPath);
            return false;
        }
    }

    if (const auto& spawnPosTxt = commandline.singleValue("spawnPos"))
    {
        InplaceArray<StringView, 3> parts;
        spawnPosTxt.view().slice(";", false, parts);

        if (parts.size() == 3)
        {
            outData.overrideSpawnPosition = true;
            parts[0].match(outData.spawnPosition.x);
            parts[1].match(outData.spawnPosition.y);
            parts[2].match(outData.spawnPosition.z);
        }
    }

    if (const auto& spawnRotTxt = commandline.singleValue("spawnRot"))
    {
        InplaceArray<StringView, 3> parts;
        spawnRotTxt.view().slice(";", false, parts);

        if (parts.size() >= 2)
        {
            parts[0].match(outData.spawnRotation.yaw);
            parts[1].match(outData.spawnRotation.pitch);
        }
        else if (parts.size() == 1)
        {
            parts[0].match(outData.spawnRotation.yaw);
        }
    }

    return true;
}

bool LauncherApp::createGame(const CommandLine& commandline)
{
    // parse parameters
    GameScreenSinglePlayerSetup spawnParams;
    if (!ParseGameStartInfo(commandline, spawnParams))
        return false;

    // create the screen stack
    const auto platform = GetService<GamePlatformService>()->platform();
    m_screenStack = RefNew<GameScreenStack>(platform);

    // should we skip all the logo BS
    const auto noSplash = commandline.hasParam("nosplash");

    // enter either the start menu or the initial panel
    const auto settings = GetSettings<GameProjectSettingScreens>();
    if (spawnParams.worldPath)
    {
        if (!settings->m_singlePlayerGameScreen)
        {
            TRACE_ERROR("No single player game screen has been defined");
            return false;
        }

        const auto worldScreen = settings->m_singlePlayerGameScreen->create<IGameScreenSinglePlayer>();
        if (!worldScreen->initialize(spawnParams))
        {
            TRACE_ERROR("Unable to initialize world screen");
            return false;
        }

        m_screenStack->pushTransition(worldScreen, GameTransitionMode::ReplaceAll, true);
    }
    else if (!noSplash && settings->m_initialScreen)
    {
        auto screen = settings->m_initialScreen->create<IGameScreen>();
        m_screenStack->pushTransition(screen, GameTransitionMode::ReplaceAll, true);
    }
    else if (settings->m_startScreen)
    {
        auto screen = settings->m_startScreen->create<IGameScreen>();
        m_screenStack->pushTransition(screen, GameTransitionMode::ReplaceAll, true);
    }
    else if (settings->m_mainMenuScreen)
    {
        auto screen = settings->m_mainMenuScreen->create<IGameScreen>();
        m_screenStack->pushTransition(screen, GameTransitionMode::ReplaceAll, true);
    }
    else
    {
        TRACE_ERROR("No initial screen defined for project!");
        return false;
    }

    return true;
}

bool LauncherApp::processInput(const InputEvent& evt)
{
    if (const auto* key = evt.toKeyEvent())
    {
        if (key->pressed() && key->keyCode() == InputKey::KEY_RETURN && key->keyMask().isLeftAltDown())
        {
            toggleFullscreen();
            return true;
        }
    }

    if (m_screenStack->input(evt))
        return true;

    return false;
}

void LauncherApp::updateWindow()
{
    // if the window wants to be closed allow it and close our app as well
    if (m_renderingOutput->window()->windowHasCloseRequest())
        GetLaunchPlatform().requestExit("Main window closed");

    // process input from the window
    if (auto inputContext = m_renderingOutput->window()->windowGetInputContext())
    {
        // process all events as they come
        while (auto inputEvent = inputContext->pull())
            processInput(*inputEvent);

        // capture input to the window if required
        inputContext->requestCapture(m_screenStack->shouldCaptureInput() ? 2 : 0);
    }
}

void LauncherApp::updateGame(double dt)
{
    // update the game state stack
    if (!m_screenStack->update(dt))
        GetLaunchPlatform().requestExit("Game stack empty");
}

//--

void LauncherApp::renderFrame()
{
    gpu::CommandWriter cmd("AppFrame");

    if (auto output = cmd.opAcquireOutput(m_renderingOutput))
    {
        //cmd.opClearRenderTarget(output.color, Vector4(0.05f, 0.05f, 0.05f, 1.0f));
        //cmd.opClearDepthStencil(output.depth, true, true, 1.0f, 0);

        {
            gpu::FrameBuffer fb;
            fb.color[0].view(output.color).clear(0.05f, 0.05f, 0.05f, 1.0f);
            fb.depth.view(output.depth).clearDepth().clearStencil();

            cmd.opBeingPass(fb);
            cmd.opEndPass();
        }

        m_screenStack->render(cmd, output);

        cmd.opSwapOutput(m_renderingOutput);
    }

    GetService<DeviceService>()->device()->submitWork(cmd.release());
}

//--

END_BOOMER_NAMESPACE()

