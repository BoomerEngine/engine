/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "public.h"

#include <PxPhysicsAPI.h>
#include <PxFoundation.h>
#include <PxScene.h>
#include <PxActor.h>

namespace physics
{
    extern base::mem::PoolID POOL_PHYSICS_COLLISION;
    extern base::mem::PoolID POOL_PHYSICS_TEMP;
    extern base::mem::PoolID POOL_PHYSICS_SCENE;
    extern base::mem::PoolID POOL_PHYSICS_RUNTIME;

    namespace runtime
    {
        class PhysicsScene;
        class PhysicsService;

    } // runtime
} // rendering