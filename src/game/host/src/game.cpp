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

#include "base/world/include/worldEntity.h"

namespace game
{
    //--

    RTTI_BEGIN_TYPE_ENUM(GameWorldStackOperation);
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

        processWorldTransition(loadingScreenState);

        processWorldStreaming(loadingScreenState);

        if (m_requestedClose)
            return false;

        if (m_worlds.empty() && !m_currentWorldTransition && !m_pendingWorldTransition)
            return false;

        return true;
    }

    bool Game::processRender(GameViewport& outRendering)
    {
        if (m_currentWorld)
        {
            if (m_currentWorld->calculateCamera(outRendering.cameraPlacement))
            {
                outRendering.world = m_currentWorld;
                return true;
            }
        }

        return false;
    }

    void Game::processRenderCanvas(base::canvas::Canvas& canvas)
    {
        if (m_currentWorld)
            m_currentWorld->renderCanvas(canvas);
    }

    bool Game::processInput(const base::input::BaseEvent& evt)
    {
        if (m_currentWorld)
        {
            if (m_currentWorld->processInputEvent(evt))
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

        if (!m_currentWorld)
            return true;

        if (m_currentWorldTransition && m_currentWorldTransition->showLoadingScreen)
            return true;

        return false;
    }

    void Game::cancelPendingWorldTransitions()
    {
        if (m_currentWorldTransition)
        {
            m_currentWorldTransition->flagCanceled = true;
            m_currentWorldTransition.reset();
        }

        m_pendingWorldTransition.reset();
    }

    void Game::scheduleWorldTransition(const GameTransitionInfo& info)
    {
        DEBUG_CHECK_RETURN_EX(!info.world || m_worlds.contains(info.world), "World is not on world list");

        if (m_currentWorld != info.world)
        {
            auto loader = base::RefNew<GameWorldTransition>();
            loader->transitionData = base::CreateUniquePtr<GameTransitionInfo>(info);
            loader->showLoadingScreen = info.showLoadingScreen;
            loader->op = info.op;
            m_pendingWorldTransition = loader;
        }
    }

    void Game::scheduleNewWorldTransition(const GameStartInfo& info)
    {
        auto loader = base::RefNew<GameWorldTransition>();
        loader->initData = base::CreateUniquePtr<GameStartInfo>(info);
        loader->showLoadingScreen = info.showLoadingScreen;
        loader->op = info.op;
        m_pendingWorldTransition = loader;
    }

    void Game::processWorldTransition(GameLoadingScreenState loadingScreenState)
    {
        /// finish current transition
        if (m_currentWorldTransition)
        {
            if (m_currentWorldTransition->flagFinished)
            {
                // NOTE: transition can finalize only if it does not require a loading screen or we are under a full one
                if (!m_currentWorldTransition->showLoadingScreen || (loadingScreenState == GameLoadingScreenState::Visible))
                {
                    const auto elapsed = m_currentWorldTransitionStart.timeTillNow().toSeconds();
                    if (m_currentWorldTransition->flagCanceled)
                    {
                        TRACE_WARNING("World transition failed after {}", TimeInterval(elapsed));
                    }
                    else
                    {
                        TRACE_INFO("World transition finished after {}", TimeInterval(elapsed));
                        applyWorldTransition(*m_currentWorldTransition);
                    }

                    m_currentWorldTransition.reset();
                }
            }
        }

        /// starting new transition, allowed always
        if (!m_currentWorldTransition && m_pendingWorldTransition)
        {
            m_currentWorldTransitionStart.resetToNow();
            m_currentWorldTransition = std::move(m_pendingWorldTransition);

            // TODO: background task ?

            auto transition = m_currentWorldTransition;

            RunFiber("WorldTransition") << [transition](FIBER_FUNC)
            {
                transition->process();
                transition->flagFinished = true;
            };
        }
    }

    void Game::applyWorldTransition(const GameWorldTransition& setup)
    {
        // cleanup before attachment
        if (setup.op == GameWorldStackOperation::ReplaceAll)
        {
            m_worlds.clear();
            m_currentWorld.reset();
        }
        else if (setup.op == GameWorldStackOperation::Replace)
        {
            m_worlds.remove(m_currentWorld);
            m_currentWorld.reset();
        }

        // add world to the list, always
        if (setup.world && !m_worlds.contains(setup.world))
            m_worlds.pushBack(setup.world);

        // set new current world ?
        if (setup.op != GameWorldStackOperation::Nop && setup.world)
            m_currentWorld = setup.world;
    }

    //--

    void Game::processWorldStreaming(GameLoadingScreenState loadingScreenState)
    {
        // finish current streaming
        if (m_currentWorldStreaming && m_currentWorldStreaming->flagFinished)
        {
            // apply the results
            if (auto world = m_currentWorldStreaming->world.lock())
            {
                if (m_worlds.contains(world))
                    world->applyStreamingTask(m_currentWorldStreaming->task);
            }

            m_currentWorldStreaming.reset();
        }

        // start new streaming task
        //if (loadingScreenState == )
        if (!m_currentWorldStreaming && m_currentWorld)
        {
            if (auto task = m_currentWorld->createStreamingTask())
            {
                auto wrapper = base::RefNew<LocalStreamingTask>();
                wrapper->world = m_currentWorld;
                wrapper->task = task;

                RunFiber("WorldStreaming") << [wrapper](FIBER_FUNC)
                {
                    wrapper->task->process();
                    wrapper->flagFinished = true;
                };

                m_currentWorldStreaming = wrapper;
            }
        }
    }

    //--

} // ui
