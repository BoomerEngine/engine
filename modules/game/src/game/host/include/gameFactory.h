/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once
#include "base/script/include/scriptObject.h"

namespace game
{

    //--

    /// initialization params for the game stack
    struct GAME_HOST_API GameInitData
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(GameInitData);

        base::Vector3 spawnPositionOverride;
        base::Angles spawnRotationOverride;
        bool spawnPositionOverrideEnabled = false;
        bool spawnRotationOverrideEnabled = false;

        base::StringBuf worldPathOverride;
    };


    //--

    /// This class spawns the game from setup data
    /// NOTE: technically there should be only one game factory per project but it can be override with project settings and with commandline
    class GAME_HOST_API IGameFactory : public base::script::ScriptedObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IGameFactory, base::script::ScriptedObject);

    public:
        IGameFactory();
        virtual ~IGameFactory();

        //--

        /// create game (with initial screen setup hopefully) for given initialization data
        CAN_YIELD virtual GamePtr createGame(const GameInitData& initData) = 0;

        //--
    };

    //--

} // game
