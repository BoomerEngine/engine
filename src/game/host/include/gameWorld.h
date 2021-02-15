/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "base/world/include/world.h"

namespace game
{
    //--

    /// A scene that can be used in game
    class GAME_HOST_API World : public base::world::World
    {
        RTTI_DECLARE_VIRTUAL_CLASS(World, base::world::World);

    public:
        World();
        virtual ~World();
    };

    //--

} // game
