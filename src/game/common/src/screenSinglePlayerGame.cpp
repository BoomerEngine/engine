/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"

#include "player.h"
#include "playerEntity.h"
#include "game.h"

#include "settings.h"
#include "screen.h"
#include "screenStack.h"
#include "screenSinglePlayerGame.h"

#include "engine/world/include/compiledWorldData.h"
#include "core/app/include/projectSettingsService.h"
#include "core/input/include/inputStructures.h"

#ifdef HAS_ENGINE_WORLD_COMPILER
    #include "engine/world_compiler/include/worldCompiler.h"
#endif

BEGIN_BOOMER_NAMESPACE()

//--

static CompiledWorldPtr LoadCompiledWorld(const GameScreenSinglePlayerSetup& setup)
{
    // compiled world content has been already specified
    if (setup.worldData)
        return setup.worldData;

    // try to load compiled world
    auto compiledScene = LoadResource<CompiledWorldData>(setup.worldPath);
    if (compiledScene)
        return compiledScene;

    // try to compile the scene
#ifdef HAS_ENGINE_WORLD_COMPILER
    return CompileRawWorld(setup.worldPath, IProgressTracker::DevNull());
#else
    return nullptr;
#endif
}

//--

struct GameScreenSinglePlayerSetupLoadingState : public IReferencable
{
    GameScreenSinglePlayerSetup m_setup;
    GamePtr m_game = nullptr;
    GamePlayerLocalPtr m_player = nullptr;

    WorldPtr m_createWorld = nullptr;
    std::atomic<bool> m_finished = false;
    std::atomic<bool> m_valid = false;

    bool process()
    {
        ScopeTimer timer;

        // load the world
        auto compiledWorld = LoadCompiledWorld(m_setup);
        DEBUG_CHECK_RETURN_EX_V(compiledWorld, "No compiled world", nullptr);

        // create the runtime world
        auto world = World::CreateCompiledWorld(compiledWorld);
        DEBUG_CHECK_RETURN_EX_V(compiledWorld, "Failed to create normal world", nullptr);

        // load initial content (persistent objects - spawn points and other crap)
        world->syncStreamingLoad();

        // TODO: load world state (if savegame is presented)

        // initialize game
        if (!m_game->initialize(world))
            return false;

        // load additional content resulted from game initialization
        world->syncStreamingLoad();

        // attach player to the game
        // NOTE: this may load content for the player
        m_game->attachPlayer(m_player);

        // load additional content resulted from adding player to world
        world->syncStreamingLoad();
        world->enableStreaming(true);
        return true;
    }
};

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IGameScreenSinglePlayer);
RTTI_END_TYPE();

IGameScreenSinglePlayer::IGameScreenSinglePlayer()
{}

IGameScreenSinglePlayer::~IGameScreenSinglePlayer()
{
}

bool IGameScreenSinglePlayer::initialize(const GameScreenSinglePlayerSetup& gameSetup)
{
    // create game
    TRACE_INFO("Creating single player game...");
    auto game = createGame(gameSetup);
    if (!game)
    {
        TRACE_WARNING("Failed to create single player game");
        return false;
    }

    // TODO: load game state (if savegame presented)

    // create player
    TRACE_INFO("Creating player...");
    auto player = createLocalPlayer(gameSetup);
    if (!player)
    {
        TRACE_WARNING("Failed to create single player game");
        return false;
    }

    // TODO: load player state (if savegame presented)

    // start world loading
    auto loading = RefNew<GameScreenSinglePlayerSetupLoadingState>();
    loading->m_setup = gameSetup;
    loading->m_game = game;
    loading->m_player = player;
    m_loadingState = loading;

    // start loading on a separate thread
    RunFiber("StartGame") << [loading](FIBER_FUNC)
    {
        loading->m_valid = loading->process();
        loading->m_finished = true;
    };

    // we are ready to go, for now
    return true;
}

//--

void IGameScreenSinglePlayer::updateLoading()
{
    if (m_loadingState && m_loadingState->m_finished)
    {
        if (m_loadingState->m_valid)
        {
            m_game = m_loadingState->m_game;
            m_player = m_loadingState->m_player;
        }
        else
        {
            // TODO: try to show error screen

            auto mainMenu = GetSettings<GameProjectSettingScreens>()->m_mainMenuScreen->create<IGameScreen>();
            host()->pushTransition(mainMenu, GameTransitionMode::ReplaceAll);
        }

        m_loadingState.reset();
    }
}

//--

bool IGameScreenSinglePlayer::queryOpaqueState() const
{
    return true;
}

bool IGameScreenSinglePlayer::queryFilterAllInput() const
{
    return true;
}

bool IGameScreenSinglePlayer::queryInputCaptureState() const
{
    return true;
}

void IGameScreenSinglePlayer::handleDetached()
{
    if (m_player)
    {
        if (m_game)
            m_game->dettachPlayer(m_player);
        m_player.reset();
    }

    m_game.reset();
}

void IGameScreenSinglePlayer::handleUpdate(double dt)
{
    updateLoading();

    if (m_game)
        m_game->update(dt);
}

bool IGameScreenSinglePlayer::handleInput(const InputEvent& evt)
{
    if (auto* key = evt.toKeyEvent())
    {
        if (key->pressed() && key->keyCode() == InputKey::KEY_ESCAPE)
        {
            auto pauseMenu = GetSettings<GameProjectSettingScreens>()->m_ingamePauseScreen->create<IGameScreen>();
            host()->pushTransition(pauseMenu, GameTransitionMode::PushOnTop);

            return true;
        }
    }

    if (m_player)
        return m_player->handleInput(this, evt);

    return false;
}

void IGameScreenSinglePlayer::handleRender(gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output, float visibility)
{
    if (m_player)
        return m_player->handleRender(this, cmd, output, visibility);
}

void IGameScreenSinglePlayer::handleRenderImGuiDebugOverlay()
{
    if (m_game)
        m_game->renderImGuiDebugInterface();
}

//--

END_BOOMER_NAMESPACE()
