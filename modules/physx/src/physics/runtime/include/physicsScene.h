/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base/app/include/localService.h"

namespace physics
{
    namespace runtime
    {

        /// simulation scene for the physics
        class PHYSICS_RUNTIME_API PhysicsScene : public base::NoCopy
        {
        public:
            PhysicsScene();
            ~PhysicsScene();
        };

    } // runtime
} // physics
