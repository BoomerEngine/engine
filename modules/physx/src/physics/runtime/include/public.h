/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "physics_runtime_glue.inl"

namespace physx
{
    class PxFoundation;
    class PxPhysics;
    class PxPvd;
}

namespace physics
{
    namespace runtime
    {
        enum class PhysicsSceneType : uint8_t
        {
            Preview, // mostly for single preview windows
            Editor, // editor scene (lots of adds and removes)
            Game, // game physics
        };

        class PhysicsService;

        class PhysicsScene;
        typedef base::UniquePtr<PhysicsScene> PhysicsScenePtr;

    } // runtime
} // physics

typedef physics::runtime::PhysicsService PhysicsService;