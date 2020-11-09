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
#include "worldComponent.h"
#include "worldEntity.h"
#include "worldComponentGraph.h"

namespace base
{
    namespace world
    {
        //--

        RTTI_BEGIN_TYPE_CLASS(EntityInputEvent);
            RTTI_PROPERTY(m_name);
            RTTI_PROPERTY(m_deltaValue);
            RTTI_PROPERTY(m_absoluteValue);
        RTTI_END_TYPE();

        EntityInputEvent::EntityInputEvent()
        {}

        //--

        RTTI_BEGIN_TYPE_CLASS(Entity);
        RTTI_END_TYPE();

        Entity::Entity()
        {}

        Entity::~Entity()
        {
            DEBUG_CHECK_EX(!m_world, "Destroying attached entity");
            DEBUG_CHECK_EX(!attached(), "Destroying attached entity");
        }

        void Entity::attachComponent(Component* comp)
        {
            ASSERT(comp);
            ASSERT(!m_components.contains(comp));
            ASSERT(!comp->attached());

            ASSERT(!comp->m_entity);
            m_components.pushBack(AddRef(comp));
            comp->m_entity = this;

            // TODO: filter this more
            invalidateRoots();

            if (attached())
            {
                DEBUG_CHECK(m_world);
                comp->attachToWorld(m_world);
            }
        }

        void Entity::dettachComponent(Component* comp)
        {
            ASSERT(comp);
            ASSERT(m_components.contains(comp));

            if (attached())
            {
                DEBUG_CHECK(m_world);
                comp->detachFromWorld(m_world);
            }

            // TODO: filter this more
            invalidateRoots();

            ASSERT(this == comp->m_entity);
            comp->m_entity = nullptr;
            m_components.remove(comp); // this may delete component !
        }

        bool Entity::isRootComponent(const Component* comp) const
        {
            auto* link = comp->transformLink();
            if (!link)
                return true;

            DEBUG_CHECK(link->linked());

            auto source = link->source();
            DEBUG_CHECK(source != nullptr);

            return !source || source->entity() != this;
        }

        void Entity::invalidateRoots()
        {
            m_flags |= EntityFlagBit::DirtyRoots;
            m_rootComponents.reset();
        }

        void Entity::handlePreTick(float dt)
        {
            // TODO: scripts :)
        }

        void Entity::handlePostTick(float dt)
        {
            // TODO: scripts :)
        }

        void Entity::requestTransformUpdate()
        {
            if (m_world)
            {
                if (!m_flags.test(EntityFlagBit::DirtyTransform))
                {
                    m_flags |= EntityFlagBit::DirtyTransform;
                    m_world->scheduleEntityForTransformUpdate(this);
                }
            }
            else
            {
                handleTransformUpdate(m_absoluteTransform);
            }
        }

        void Entity::requestTransform(const AbsoluteTransform& newTransform)
        {
            if (m_world)
            {
                m_flags |= EntityFlagBit::DirtyTransform;
                m_world->scheduleEntityForTransformUpdateWithTransform(this, newTransform);
            }
            else
            {
                handleTransformUpdate(newTransform);
            }
        }

        void Entity::requestMove(const AbsolutePosition& newPosition)
        {
            auto transform = m_absoluteTransform;
            transform.position(newPosition);
            requestTransform(transform);
        }

        void Entity::requestMove(const AbsolutePosition& newPosition, const Quat& newRotation)
        {
            auto transform = m_absoluteTransform;
            transform.position(newPosition);
            transform.rotation(newRotation);
            requestTransform(transform);
        }

        void Entity::handleTransformUpdate(const AbsoluteTransform& transform)
        {
            // update entity transform
            m_absoluteTransform = transform;
            m_localToWorld = m_absoluteTransform.approximate();

            // transform is no longer invalid
            m_flags -= EntityFlagBit::DirtyTransform;

            // update root components
            if (m_flags.test(EntityFlagBit::DirtyRoots))
            {
                m_flags -= EntityFlagBit::DirtyRoots;
                m_rootComponents.reset();

                for (auto& comp : m_components)
                    if (isRootComponent(comp))
                        m_rootComponents.pushBack(comp);
            }

            for (auto* root : m_rootComponents)
                root->handleTransformUpdate(nullptr, m_absoluteTransform, m_localToWorld);
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

            //m_absoluteTransform(initialTransform)
            //  m_localToWorld = m_absoluteTransform.approximate();
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

        void Entity::handleInput(const EntityInputEvent& gameInput)
        {

        }

        IWorldSystem* Entity::systemPtr(short index) const
        {
            if (attached() && m_world)
                return m_world->systemPtr(index);
            return nullptr;
        }

        //--

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

    } // world
} // game
