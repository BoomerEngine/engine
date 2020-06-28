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

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IEntityMovement);
    RTTI_END_TYPE();

    IEntityMovement::~IEntityMovement()
    {}

    void IEntityMovement::handleAttach(Entity* entity, const IEntityMovement* currentMovement)
    {
        // nothing
    }

    void IEntityMovement::handleDetach(Entity* entity)
    {
        // nothing
    }

    void IEntityMovement::requestMove(const base::AbsolutePosition& pos, const base::Quat& rot)
    {
        // nothing
    }

    void IEntityMovement::attachToEntity(Entity* entity, const IEntityMovement* old)
    {
        DEBUG_CHECK_EX(entity != nullptr, "Cannot attach to empty entity");
        DEBUG_CHECK_EX(m_owner.empty(), "Already attached to entity");

        if (!m_owner && entity)
        {
            m_owner = entity;
            handleAttach(entity, old);
        }
    }

    void IEntityMovement::detachFromEntity(Entity* entity)
    {
        DEBUG_CHECK_EX(m_owner == entity, "Not attached to given entity");
        if (m_owner == entity)
            handleDetach(entity);
        m_owner = nullptr;
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(EntityMovement_Teleport);
    RTTI_END_TYPE();

    EntityMovement_Teleport::EntityMovement_Teleport()
    {}

    void EntityMovement_Teleport::handleAttach(Entity* entity, const IEntityMovement* currentMovement)
    {
        TBaseClass::handleAttach(entity, currentMovement);

        m_position = entity->absoluteTransform().position();
        m_rotation = entity->absoluteTransform().rotation();
    }

    void EntityMovement_Teleport::requestMove(const base::AbsolutePosition& pos, const base::Quat& rot)
    {
        m_position = pos;
        m_rotation = rot;
    }

    void EntityMovement_Teleport::calculateAbsoluteTransform(base::AbsoluteTransform& outTransform) const
    {
        outTransform.position(m_position);
        outTransform.rotation(m_rotation);
    }

    //--

} // game
