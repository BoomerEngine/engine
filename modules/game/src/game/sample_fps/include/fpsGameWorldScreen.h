/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: fps #]
***/

#pragma once

#include "game/host/include/gameScreen.h"
#include "rendering/scene/include/renderingSceneStats.h"

namespace fps
{

    /// a game screen that let's you play on the world
    class GAME_SAMPLE_FPS_API WorldScreen : public game::IScreen
    {
        RTTI_DECLARE_VIRTUAL_CLASS(WorldScreen, game::IScreen);

    public:
        WorldScreen(const game::WorldPtr& world);

    protected:
        virtual void handleUpdate(game::IGame* game, double dt) override;
        virtual void handleEvent(game::IGame* game, const game::EventPtr& evt) override;
        virtual void handleRender(game::IGame* game, rendering::command::CommandWriter& cmd, const game::HostViewport& viewport) override;
        virtual bool handleInput(game::IGame* game, const base::input::BaseEvent& evt) override;
        virtual void handleStartHide(game::IGame* game) override;
        virtual void handleStartShow(game::IGame* game) override;
        virtual void handleDebug() override;

        //--

        game::WorldPtr m_world;

        rendering::scene::FrameStats m_lastFrameStats;
        rendering::scene::SceneStats m_lastSceneStats;

        uint32_t m_frameIndex = 0;
        double m_totalTime = 0.0;
    };

} // fps