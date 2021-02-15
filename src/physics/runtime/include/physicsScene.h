/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base/script/include/scriptObject.h"

namespace physics
{
    namespace runtime
    {

        /// simulation scene for the physics
        class PHYSICS_RUNTIME_API PhysicsScene : public base::script::ScriptedObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(PhysicsScene, base::script::ScriptedObject);

        public:
            PhysicsScene(physx::PxScene* scene);
            ~PhysicsScene();

            // simulate with given time delta
            void simulate(float dt);

        private:
            physx::PxScene* m_scene = nullptr;
        };

    } // runtime
} // physics