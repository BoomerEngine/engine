/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core/app/include/localService.h"

BEGIN_BOOMER_NAMESPACE()

/// scene desc
struct ENGINE_PHYSICS_RUNTIME_API PhysicsSceneDesc
{
    PhysicsSceneType m_type;

    PhysicsSceneDesc();
};

/// physics service for the engine
class ENGINE_PHYSICS_RUNTIME_API PhysicsService : public IService
{
    RTTI_DECLARE_VIRTUAL_CLASS(PhysicsService, IService);

public:
    PhysicsService();

    /// create a physics scene
    PhysicsScenePtr createScene(const PhysicsSceneDesc& desc);

private:
    virtual bool onInitializeService(const CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;

    physx::PxFoundation* m_foundation;
    physx::PxPhysics* m_physics;
    physx::PxPvd* m_pvd;
};

END_BOOMER_NAMESPACE()
