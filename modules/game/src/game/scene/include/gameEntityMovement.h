/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity #]
*
***/

#pragma once

#include "base/script/include/scriptObject.h"

namespace game
{

    //---

    // entity movement handler - allows entity to move and facilitates the movement
    class GAME_SCENE_API IEntityMovement : public base::script::ScriptedObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IEntityMovement, base::script::ScriptedObject);

    public:
        virtual ~IEntityMovement();

        /// entity we are attached to
        INLINE const EntityWeakPtr& entity() const { return m_owner; }

        //---

        /// attach movement to entity
        virtual void handleAttach(Entity* entity, const IEntityMovement* currentMovement);

        /// detach movement controller from entity
        virtual void handleDetach(Entity* entity);

        //---

        /// request entity to be moved to given location/rotation, can work or not, it's a REQUEST, exact details depend on the implementation
        virtual void requestMove(const base::AbsolutePosition& pos, const base::Quat& rot);

        /// calculate movement target
        virtual void calculateAbsoluteTransform(base::AbsoluteTransform& outTransform) const = 0;

    private:
        EntityWeakPtr m_owner;

        void attachToEntity(Entity* entity, const IEntityMovement* old);
        void detachFromEntity(Entity* entity);

        friend class Entity;
    };

    //---

    // simplest movement - teleporting from one place to the other
    class GAME_SCENE_API EntityMovement_Teleport : public IEntityMovement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(EntityMovement_Teleport, IEntityMovement);

    public:
        EntityMovement_Teleport();

        virtual void handleAttach(Entity* entity, const IEntityMovement* currentMovement) override;

        virtual void requestMove(const base::AbsolutePosition& pos, const base::Quat& rot) override;
        virtual void calculateAbsoluteTransform(base::AbsoluteTransform& outTransform) const  override;

    private:
        base::AbsolutePosition m_position;
        base::Quat m_rotation;
    };

    //---

} // game

