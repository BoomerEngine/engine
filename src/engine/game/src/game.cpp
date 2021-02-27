/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "game.h"
#include "gameWorld.h"
#include "gameWorldLoader.h"

#include "engine/world/include/entity.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_ENUM(GameGameWorldStackOperation);
    RTTI_ENUM_OPTION(Nop); // 
    RTTI_ENUM_OPTION(Activate);
    RTTI_ENUM_OPTION(Replace);
    RTTI_ENUM_OPTION(ReplaceAll);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_CLASS(GameStartInfo)
    RTTI_PROPERTY(op);
    RTTI_PROPERTY(showLoadingScreen);
    RTTI_PROPERTY(scenePath);
    RTTI_PROPERTY(spawnPosition);
    RTTI_PROPERTY(spawnRotation);
    RTTI_PROPERTY(spawnOverrideEnabled);
RTTI_END_TYPE();

GameStartInfo::GameStartInfo()
{}

//----

RTTI_BEGIN_TYPE_CLASS(GameTransitionInfo);
    RTTI_PROPERTY(op);
    RTTI_PROPERTY(showLoadingScreen);
    RTTI_PROPERTY(world);
RTTI_END_TYPE();

GameTransitionInfo::GameTransitionInfo()
{}

//----

RTTI_BEGIN_TYPE_CLASS(GameViewport);
    RTTI_PROPERTY(world);
    RTTI_PROPERTY(cameraPlacement);
RTTI_END_TYPE();

//----

RTTI_BEGIN_TYPE_CLASS(Game);
RTTI_END_TYPE();

Game::Game()
{
}

Game::~Game()
{
    m_worlds.clear();
}

bool Game::processUpdate(double dt, GameLoadingScreenState loadingScreenState)
{
    for (auto& scene : m_worlds)
        scene->update(dt);

    processGameWorldTransition(loadingScreenState);

    processGameWorldStreaming(loadingScreenState);

    if (m_requestedClose)
        return false;

    if (m_worlds.empty() && !m_currentGameWorldTransition && !m_pendingGameWorldTransition)
        return false;

    return true;
}

bool Game::processRender(GameViewport& outRendering)
{
    if (m_currentGameWorld)
    {
        if (m_currentGameWorld->calculateCamera(outRendering.cameraPlacement))
        {
            outRendering.world = m_currentGameWorld;
            return true;
        }
    }

    return false;
}

void Game::processRenderCanvas(canvas::Canvas& canvas)
{
    if (m_currentGameWorld)
        m_currentGameWorld->renderCanvas(canvas);
}

bool Game::processInput(const input::BaseEvent& evt)
{
    if (m_currentGameWorld)
    {
        if (m_currentGameWorld->processInputEvent(evt))
            return true;
    }

    return false;
}

void Game::requestClose()
{
    if (!m_requestedClose)
    {
        TRACE_INFO("Game requested close");
        m_requestedClose = true;
    }
}

//--

bool Game::requiresLoadingScreen() const
{
    if (!m_worlds.empty())
        return true;

    if (!m_currentGameWorld)
        return true;

    if (m_currentGameWorldTransition && m_currentGameWorldTransition->showLoadingScreen)
        return true;

    return false;
}

void Game::cancelPendingGameWorldTransitions()
{
    if (m_currentGameWorldTransition)
    {
        m_currentGameWorldTransition->flagCanceled = true;
        m_currentGameWorldTransition.reset();
    }

    m_pendingGameWorldTransition.reset();
}

void Game::scheduleGameWorldTransition(const GameTransitionInfo& info)
{
    DEBUG_CHECK_RETURN_EX(!info.world || m_worlds.contains(info.world), "GameWorld is not on world list");

    if (m_currentGameWorld != info.world)
    {
        auto loader = RefNew<GameGameWorldTransition>();
        loader->transitionData = CreateUniquePtr<GameTransitionInfo>(info);
        loader->showLoadingScreen = info.showLoadingScreen;
        loader->op = info.op;
        m_pendingGameWorldTransition = loader;
    }
}

void Game::scheduleNewGameWorldTransition(const GameStartInfo& info)
{
    auto loader = RefNew<GameGameWorldTransition>();
    loader->initData = CreateUniquePtr<GameStartInfo>(info);
    loader->showLoadingScreen = info.showLoadingScreen;
    loader->op = info.op;
    m_pendingGameWorldTransition = loader;
}

void Game::processGameWorldTransition(GameLoadingScreenState loadingScreenState)
{
    /// finish current transition
    if (m_currentGameWorldTransition)
    {
        if (m_currentGameWorldTransition->flagFinished)
        {
            // NOTE: transition can finalize only if it does not require a loading screen or we are under a full one
            if (!m_currentGameWorldTransition->showLoadingScreen || (loadingScreenState == GameLoadingScreenState::Visible))
            {
                const auto elapsed = m_currentGameWorldTransitionStart.timeTillNow().toSeconds();
                if (m_currentGameWorldTransition->flagCanceled)
                {
                    TRACE_WARNING("GameWorld transition failed after {}", TimeInterval(elapsed));
                }
                else
                {
                    TRACE_INFO("GameWorld transition finished after {}", TimeInterval(elapsed));
                    applyGameWorldTransition(*m_currentGameWorldTransition);
                }

                m_currentGameWorldTransition.reset();
            }
        }
    }

    /// starting new transition, allowed always
    if (!m_currentGameWorldTransition && m_pendingGameWorldTransition)
    {
        m_currentGameWorldTransitionStart.resetToNow();
        m_currentGameWorldTransition = std::move(m_pendingGameWorldTransition);

        // TODO: background task ?

        auto transition = m_currentGameWorldTransition;

        RunFiber("GameWorldTransition") << [transition](FIBER_FUNC)
        {
            transition->process();
            transition->flagFinished = true;
        };
    }
}

void Game::applyGameWorldTransition(const GameGameWorldTransition& setup)
{
    // cleanup before attachment
    if (setup.op == GameGameWorldStackOperation::ReplaceAll)
    {
        m_worlds.clear();
        m_currentGameWorld.reset();
    }
    else if (setup.op == GameGameWorldStackOperation::Replace)
    {
        m_worlds.remove(m_currentGameWorld);
        m_currentGameWorld.reset();
    }

    // add world to the list, always
    if (setup.world && !m_worlds.contains(setup.world))
        m_worlds.pushBack(setup.world);

    // set new current world ?
    if (setup.op != GameGameWorldStackOperation::Nop && setup.world)
        m_currentGameWorld = setup.world;
}

//--

void Game::processGameWorldStreaming(GameLoadingScreenState loadingScreenState)
{
    // finish current streaming
    if (m_currentGameWorldStreaming && m_currentGameWorldStreaming->flagFinished)
    {
        // apply the results
        if (auto world = m_currentGameWorldStreaming->world.lock())
        {
            if (m_worlds.contains(world))
                world->applyStreamingTask(m_currentGameWorldStreaming->task);
        }

        m_currentGameWorldStreaming.reset();
    }

    // start new streaming task
    //if (loadingScreenState == )
    if (!m_currentGameWorldStreaming && m_currentGameWorld)
    {
        if (auto task = m_currentGameWorld->createStreamingTask())
        {
            auto wrapper = RefNew<LocalStreamingTask>();
            wrapper->world = m_currentGameWorld;
            wrapper->task = task;

            RunFiber("GameWorldStreaming") << [wrapper](FIBER_FUNC)
            {
                wrapper->task->process();
                wrapper->flagFinished = true;
            };

            m_currentGameWorldStreaming = wrapper;
        }
    }
}

//--

END_BOOMER_NAMESPACE()
