/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "gameSystem.h"

namespace game
{
    //----

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IGameSystem);
    RTTI_END_TYPE();

    IGameSystem::IGameSystem()
    {}

    IGameSystem::~IGameSystem()
    {}

    void IGameSystem::handleInitialize(IGame* game)
    {}

    void IGameSystem::handlePreUpdate(IGame* game, double dt)
    {}

    void IGameSystem::handlePostUpdate(IGame* game, double dt)
    {}

    void IGameSystem::handleRenderDebug(IGame* game, rendering::command::CommandWriter& cmd, const HostViewport& viewport)
    {}

    void IGameSystem::handleDebug(IGame* game)
    {}

    //---

} // ui
