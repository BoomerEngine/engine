/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
*
***/

#include "build.h"
#include "sceneRuntime.h"
#include "sceneRuntimeSystem.h"
#include "sceneEntity.h"
#include "sceneEntityTickSystem.h"

namespace scene
{

    ///---

    RTTI_BEGIN_TYPE_CLASS(EntityTickSystem);
        RTTI_METADATA(scene::RuntimeSystemInitializationOrderMetadata).order(0);
    RTTI_END_TYPE();

    EntityTickSystem::EntityTickSystem()
        : m_isTicking(false)
    {}

    EntityTickSystem::~EntityTickSystem()
    {}

    void EntityTickSystem::registerEntity(Entity* entity)
    {
        ASSERT(entity);

        if (m_isTicking)
        {
            m_unregisteredDuringUpdate.remove(entity);

            ASSERT(!m_registeredDuringUpdate.contains(entity));
            m_registeredDuringUpdate.pushBack(entity);
            entity->addManualStrongRef();
        }
        else
        {
            ASSERT(!m_entities.contains(entity));
            m_entities.insert(entity);
            entity->addManualStrongRef();
        }
    }

    void EntityTickSystem::unregisterEntity(Entity* entity)
    {
        ASSERT(entity);

        if (m_isTicking)
        {
            if (m_registeredDuringUpdate.remove(entity))
                entity->removeManualStrongRef();

            ASSERT(!m_unregisteredDuringUpdate.contains(entity));
            m_unregisteredDuringUpdate.pushBack(entity);
        }
        else
        {
            ASSERT(m_entities.contains(entity));
            m_entities.remove(entity);
            entity->removeManualStrongRef();
        }
    }

    void EntityTickSystem::onPreTick(Scene& scene, const UpdateContext& ctx)
    {

    }

    void EntityTickSystem::onTick(Scene& scene, const UpdateContext& ctx)
    {
        PC_SCOPE_LVL0(EntityTick);

        ASSERT(!m_isTicking);
        m_isTicking = true;

        for (auto entity  : m_entities)
            entity->update(ctx.m_dt);

        m_isTicking = false;

        for (auto ptr  : m_unregisteredDuringUpdate)
            unregisterEntity(ptr);

        for (auto ptr  : m_registeredDuringUpdate)
        {
            registerEntity(ptr);
            ptr->removeManualStrongRef();
        }
    }

    void EntityTickSystem::onPreTransform(Scene& scene, const UpdateContext& ctx)
    {

    }

    void EntityTickSystem::onTransform(Scene& scene, const UpdateContext& ctx)
    {

    }

    void EntityTickSystem::onPostTransform(Scene& scene, const UpdateContext& ctx)
    {

    }

    void EntityTickSystem::onRenderFrame(Scene& scene, rendering::scene::FrameInfo& info)
    {

    }

    ///---

} // scene

