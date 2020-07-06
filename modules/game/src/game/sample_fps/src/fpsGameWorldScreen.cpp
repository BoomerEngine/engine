/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: fps #]
***/

#include "build.h"
#include "fpsGameWorldScreen.h"

#include "game/world/include/world.h"
#include "game/host/include/gameHost.h"
#include "rendering/scene/include/renderingFrameParams.h"
#include "rendering/scene/include/renderingFrameRenderingService.h"
#include "rendering/driver/include/renderingCommandWriter.h"

namespace fps
{
    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(WorldScreen);
    RTTI_END_TYPE();

    //--

    WorldScreen::WorldScreen(const game::WorldPtr& world)
        : m_world(world)
    {}

    void WorldScreen::handleUpdate(game::IGame* game, double dt)
    {
        m_totalTime += dt;
        m_world->update(dt);
    }

    void WorldScreen::handleEvent(game::IGame* game, const game::EventPtr& evt)
    {
    }

    void WorldScreen::handleRender(game::IGame* game, rendering::command::CommandWriter& cmd, const game::HostViewport& viewport)
    {
        rendering::scene::FrameParams params(viewport.width, viewport.height, rendering::scene::Camera());

        params.filters = rendering::scene::FilterFlags::DefaultGame();
        params.mode = rendering::scene::FrameRenderMode::Default;
        params.index = m_frameIndex++;
        params.resolution.width = viewport.width;
        params.resolution.height = viewport.height;
        params.resolution.finalCompositionWidth = viewport.width;
        params.resolution.finalCompositionHeight = viewport.height;
        params.time.gameTime = m_totalTime;
        params.time.engineRealTime = m_totalTime;
        params.cascades.numCascades = 3;

        m_world->render(params);

        if (auto sceneRenderingCommands = base::GetService<rendering::scene::FrameRenderingService>()->renderFrame(params, viewport.backBufferColor, &m_lastFrameStats, &m_lastSceneStats))
            cmd.opAttachChildCommandBuffer(sceneRenderingCommands);
    }

    bool WorldScreen::handleInput(game::IGame* game, const base::input::BaseEvent& evt)
    {
        if (m_world)
            return m_world->processInput(evt);
        return false;
    }

    void WorldScreen::handleStartHide(game::IGame* game)
    {}

    void WorldScreen::handleStartShow(game::IGame* game)
    {}

    void WorldScreen::handleDebug()
    {
        m_world->renderDebugGui();
    }

    //--

} // fps