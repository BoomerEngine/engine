/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "gameInputSystem.h"

namespace game
{
    //----

    RTTI_BEGIN_TYPE_CLASS(GameInputSystem);
    RTTI_END_TYPE();

    //---

    GameInputSystem::GameInputSystem()
    {}

    GameInputSystem::~GameInputSystem()
    {
    }

    void GameInputSystem::handleInitialize(IGame* game)
    {
    }

    void GameInputSystem::handlePreUpdate(IGame* game, double dt)
    {
    }

    void GameInputSystem::handleDebug(IGame* game)
    {
    }

    //--

} // ui
