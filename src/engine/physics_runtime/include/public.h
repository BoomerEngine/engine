/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "engine_physics_runtime_glue.inl"

namespace physx
{
    class PxFoundation;
    class PxPhysics;
    class PxScene;
    class PxPvd;
}

BEGIN_BOOMER_NAMESPACE()

enum class PhysicsSceneType : uint8_t
{
    Preview, // mostly for single preview windows
    Editor, // editor scene (lots of adds and removes)
    Game, // game physics
};

class PhysicsService;

class PhysicsScene;
typedef UniquePtr<PhysicsScene> PhysicsScenePtr;

END_BOOMER_NAMESPACE();