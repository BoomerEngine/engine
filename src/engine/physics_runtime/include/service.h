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
class ENGINE_PHYSICS_RUNTIME_API PhysicsService : public app::ILocalService
{
    RTTI_DECLARE_VIRTUAL_CLASS(PhysicsService, app::ILocalService);

public:
    PhysicsService();

    /// create a physics scene
    PhysicsScenePtr createScene(const PhysicsSceneDesc& desc);

private:
    virtual app::ServiceInitializationResult onInitializeService(const app::CommandLine& cmdLine) override final;
    virtual void onShutdownService() override final;
    virtual void onSyncUpdate() override final;

    physx::PxFoundation* m_foundation;
    physx::PxPhysics* m_physics;
    physx::PxPvd* m_pvd;
};

END_BOOMER_NAMESPACE()
