/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core/object/include/object.h"

BEGIN_BOOMER_NAMESPACE()

/// simulation scene for the physics
class ENGINE_PHYSICS_RUNTIME_API PhysicsScene : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(PhysicsScene, IObject);

public:
    PhysicsScene(physx::PxScene* scene);
    ~PhysicsScene();

    // simulate with given time delta
    void simulate(float dt);

private:
    physx::PxScene* m_scene = nullptr;
};

END_BOOMER_NAMESPACE()
