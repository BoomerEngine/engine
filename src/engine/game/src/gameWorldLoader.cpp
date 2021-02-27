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

#include "engine/world/include/compiledScene.h"
#include "engine/world/include/streamingSystem.h"
#include "engine/world/include/entity.h"
#include "engine/world/include/world.h"

BEGIN_BOOMER_NAMESPACE()

//--

void GameGameWorldTransition::process()
{
    ScopeTimer timer;

    if (transitionData)
    {
        if (!processTransition(*transitionData))
            flagCanceled = true;
    }
    else if (initData)
    {
        if (!processInit(*initData))
            flagCanceled = true;
    }

    TRACE_INFO("Total transition processing time: {}", timer);
}

bool GameGameWorldTransition::processTransition(const GameTransitionInfo& setup)
{
    return false;
}

bool GameGameWorldTransition::processInit(const GameStartInfo& setup)
{
    // load the scene
    auto content = LoadResource<CompiledScene>(setup.scenePath);
    if (!content)
    {
        TRACE_ERROR("Unable to load compiled scene '{}'", setup.scenePath);
        return false;
    }

    // create world if needed
    if (!world)
        world = RefNew<GameWorld>();

    // bind loaded scene to the streaming system
    auto streaming = world->system<StreamingSystem>();
    streaming->bindScene(content);

    // create the initial streaming task with no observers - this will load the "always loaded" content
    {
        Array<StreamingObserverInfo> observers;
        if (auto task = streaming->createStreamingTask(observers))
        {
            ScopeTimer timer;
            task->process();
            streaming->applyStreamingTask(task);

            TRACE_INFO("Loaded initial content in {}", timer);
        }
    }

    // update the scene - this might spawn more work
    // NOTE: we don't have game yet
    for (uint32_t i = 0; i < 5; ++i)
    {
        const float dt = 0.1f;
        world->update(dt);
    }

    // TODO: find the "Game rules" entity and initialize it 
    // NOTE: this may create more entities
    {
        // TODO: tick more 
    }

    // override position
    if (world->viewStack().empty())
    {
        if (setup.spawnOverrideEnabled)
            world->toggleFreeCamera(true, &setup.spawnPosition, &setup.spawnRotation);
        else
            world->toggleFreeCamera(true);
    }

    // create normal streaming task for the world
    {
        if (auto task = world->createStreamingTask())
        {
            ScopeTimer timer;
            task->process();
            streaming->applyStreamingTask(task);

            TRACE_INFO("Loaded view dependent content in {}", timer);
        }
    }

    // TODO: wait for all world systems to report "ready" (spawning NPCs, etc)

    return true;
}

//--

END_BOOMER_NAMESPACE()


