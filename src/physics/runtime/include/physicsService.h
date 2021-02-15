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
        /// scene desc
        struct PHYSICS_RUNTIME_API PhysicsSceneDesc
        {
            PhysicsSceneType m_type;

            PhysicsSceneDesc();
        };

        /// physics service for the engine
        class PHYSICS_RUNTIME_API PhysicsService : public base::app::ILocalService
        {
            RTTI_DECLARE_VIRTUAL_CLASS(PhysicsService, base::app::ILocalService);

        public:
            PhysicsService();

            /// create a physics scene
            PhysicsScenePtr createScene(const PhysicsSceneDesc& desc);

        private:
            virtual base::app::ServiceInitializationResult onInitializeService(const base::app::CommandLine& cmdLine) override final;
            virtual void onShutdownService() override final;
            virtual void onSyncUpdate() override final;

            physx::PxFoundation* m_foundation;
            physx::PxPhysics* m_physics;
            physx::PxPvd* m_pvd;
        };

    } // runtime
} // physics
