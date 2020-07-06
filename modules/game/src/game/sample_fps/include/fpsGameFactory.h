/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: fps #]
***/

#pragma once
#include "game/host/include/gameFactory.h"

namespace fps
{

    //--

    class GAME_SAMPLE_FPS_API GameFactory : public game::IGameFactory
    {
        RTTI_DECLARE_VIRTUAL_CLASS(GameFactory, game::IGameFactory);

    public:
        GameFactory();
        virtual ~GameFactory();

        //--

        /// create game (with initial screen setup hopefully) for given initialization data
        CAN_YIELD virtual game::GamePtr createGame(const game::GameInitData& initData) override final;

        //--
    };

    //--

} // fps