/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
*
***/

#pragma once

#include "sceneRuntimeSystem.h"
#include "base/containers/include/hashSet.h"

namespace scene
{

    //---

    // entity collection
    class SCENE_COMMON_API EntityTickSystem : public IRuntimeSystem
    {
        RTTI_DECLARE_VIRTUAL_CLASS(EntityTickSystem, IRuntimeSystem);

    public:
        EntityTickSystem();
        virtual ~EntityTickSystem();

        /// register entity in the system, entity will receive ticks
        void registerEntity(Entity* entity);

        /// unregister entity from the tick system
        void unregisterEntity(Entity* entity);

    protected:
        virtual void onPreTick(Scene& scene, const UpdateContext& ctx) override final;
        virtual void onTick(Scene& scene, const UpdateContext& ctx) override final;
        virtual void onPreTransform(Scene& scene, const UpdateContext& ctx) override final;
        virtual void onTransform(Scene& scene, const UpdateContext& ctx) override final;
        virtual void onPostTransform(Scene& scene, const UpdateContext& ctx) override final;
        virtual void onRenderFrame(Scene& scene, rendering::scene::FrameInfo& info) override final;

        //--

        base::HashSet<Entity*> m_entities;

        base::Array<Entity*> m_registeredDuringUpdate;
        base::Array<Entity*> m_unregisteredDuringUpdate;
        bool m_isTicking;
    };

    //---

} // scene
