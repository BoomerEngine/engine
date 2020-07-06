/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: fps #]
***/

#pragma once

#include "game/host/include/game.h"

namespace fps
{

    /// simple FPS game demo
    class GAME_SAMPLE_FPS_API Game : public game::IGame
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Game, game::IGame);

    public:
        Game();

    protected:
        virtual void handleEarlyUpdate(double dt) override;
        virtual void handleMainUpdate(double dt) override;
        virtual void handleLateUpdate(double dt) override;
        virtual bool handleOutstandingInput(const base::input::BaseEvent& evt) override;
        virtual void handleDebug() override;
    };

} // fps