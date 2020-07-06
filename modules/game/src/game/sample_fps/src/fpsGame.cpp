/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: fps #]
***/

#include "build.h"
#include "fpsGame.h"

namespace fps
{
    //--

    RTTI_BEGIN_TYPE_CLASS(Game);
    RTTI_END_TYPE();

    //--

    Game::Game()
    {}

    void Game::handleEarlyUpdate(double dt)
    {
        TBaseClass::handleEarlyUpdate(dt);
    }

    void Game::handleMainUpdate(double dt)
    {
        TBaseClass::handleMainUpdate(dt);
    }

    void Game::handleLateUpdate(double dt)
    {
        TBaseClass::handleLateUpdate(dt);
    }

    bool Game::handleOutstandingInput(const base::input::BaseEvent& evt)
    {
        return TBaseClass::handleOutstandingInput(evt);
    }

    void Game::handleDebug()
    {
        TBaseClass::handleDebug();
    }

    //--

} // fps