/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity #]
*
***/

#include "build.h"
#include "world.h"
#include "gameComponent.h"
#include "gameEntity.h"
#include "gameEntityMovement.h"

namespace game
{
    //--

    RTTI_BEGIN_TYPE_CLASS(Entity);
    RTTI_END_TYPE();

    Entity::Entity()
    {}

    Entity::Entity(const base::AbsoluteTransform& initialTransform)
        : m_absoluteTransform(initialTransform)
    {
        m_localToWorld = m_absoluteTransform.approximate();
    }

    Entity::~Entity()
    {
        DEBUG_CHECK_EX(!m_world, "Destroying attached entity");
        DEBUG_CHECK_EX(!attached(), "Destroying attached entity");
    }

    void Entity::attachComponent(const ComponentPtr& comp)
    {
        ASSERT(comp);
        ASSERT(!m_components.contains(comp));
        ASSERT(!comp->attached());

        ASSERT(!comp->m_entity);
        m_components.pushBack(comp);
        comp->m_entity = this;

        if (attached())
        {
            DEBUG_CHECK(m_world);
            comp->attachToWorld(m_world);
        }
    }

    void Entity::dettachComponent(const ComponentPtr& comp)
    {
        ASSERT(comp);
        ASSERT(m_components.contains(comp));

        if (attached())
        {
            DEBUG_CHECK(m_world);
            comp->detachFromWorld(m_world);
        }

        ASSERT(this == comp->m_entity);
        m_components.remove(comp);
        comp->m_entity = nullptr;
    }

    void Entity::handlePreTick(float dt)
    {

    }

    void Entity::handlePostTick(float dt)
    {

    }

    void Entity::movement(IEntityMovement* movement)
    {
        if (m_movement != movement)
        {
            if (m_movement)
                m_movement->detachFromEntity(this);

            auto old = std::move(m_movement);
            m_movement = AddRef(movement);

            if (m_movement)
                m_movement->attachToEntity(this, old);
        }
    }

    void Entity::handleTransformUpdate()
    {
        // update transform with movement controller
        if (m_movement)
            m_movement->calculateAbsoluteTransform(m_absoluteTransform);

        // read transform 
        m_localToWorld = m_absoluteTransform.approximate();

        // 
        for (uint32_t i = 0; i < m_components.size(); ++i)
        {
            const auto& comp = m_components.typedData()[i];
            if (comp && comp->transformAttachment() == nullptr)
            {
                comp->handleTransformUpdate(m_absoluteTransform, nullptr);
            }
        }
    }

    void Entity::handleAttach(World* world)
    {
        ASSERT(world);
        ASSERT(m_flags.test(EntityFlagBit::Attaching));
        m_flags -= EntityFlagBit::Attaching;
        m_flags |= EntityFlagBit::Attached;
        m_world = world;

        for (auto& comp : m_components)
            comp->attachToWorld(world);
    }

    void Entity::handleDetach(World* world)
    {
        ASSERT(world);
        ASSERT(m_world == world);
        ASSERT(m_flags.test(EntityFlagBit::Detaching));

        for (auto& comp : m_components)
            comp->detachFromWorld(world);

        m_flags -= EntityFlagBit::Detaching;
        m_flags -= EntityFlagBit::Attached;
        m_world = nullptr;
    }

    void Entity::handleDebugRender(rendering::scene::FrameParams& frame) const
    {

    }

    void Entity::handleGameInput(const InputEventPtr& gameInput)
    {

    }

    IWorldSystem* Entity::systemPtr(short index) const
    {
        if (attached() && m_world)
            return m_world->systemPtr(index);
        return nullptr;
    }

    void Entity::attachToWorld(World* world)
    {
        ASSERT(!m_world);
        ASSERT(world);
        ASSERT(!m_flags.test(EntityFlagBit::Attaching));
        ASSERT(!m_flags.test(EntityFlagBit::Detaching));
        m_world = world;
        m_flags |= EntityFlagBit::Attaching;
        handleAttach(world);
        ASSERT(!m_flags.test(EntityFlagBit::Attaching));
        ASSERT(m_flags.test(EntityFlagBit::Attached));
        m_flags -= EntityFlagBit::Attaching;
        m_flags |= EntityFlagBit::Attached;
    }

    void Entity::detachFromWorld(World* world)
    {
        ASSERT(world);
        ASSERT(m_world == world);
        ASSERT(m_flags.test(EntityFlagBit::Attached));
        ASSERT(!m_flags.test(EntityFlagBit::Attaching));
        ASSERT(!m_flags.test(EntityFlagBit::Detaching));
        m_flags |= EntityFlagBit::Detaching;
        handleDetach(world);
        ASSERT(!m_flags.test(EntityFlagBit::Detaching));
        m_flags -= EntityFlagBit::Detaching;
        m_world = nullptr;
    }

    //--

} // game
