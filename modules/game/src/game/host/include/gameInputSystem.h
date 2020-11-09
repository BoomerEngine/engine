/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game\system #]
***/

#pragma once

#include "gameSystem.h"

namespace game
{
    //--

    /// Resource system for the input
    class GAME_HOST_API GameInputSystem : public IGameSystem
    {
        RTTI_DECLARE_VIRTUAL_CLASS(GameInputSystem, IGameSystem);

    public:
        GameInputSystem();
        virtual ~GameInputSystem();

        //--


        //--

    protected:
        virtual void handleInitialize(IGame* game) override final;
        virtual void handlePreUpdate(IGame* game, double dt) override final;
        virtual void handleDebug(IGame* game) override final;
    };

    //--

} // game
