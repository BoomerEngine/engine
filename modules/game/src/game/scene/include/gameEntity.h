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

    enum class EntityFlagBit : uint64_t
    {
        Hidden = FLAG(0),
        TempHidden = FLAG(1),
        NoFadeIn = FLAG(2),
        NoFadeOut = FLAG(3),
        NoDetailChange = FLAG(4),
        Selected = FLAG(5),
        Attaching = FLAG(10),
        Detaching = FLAG(11),
        Attached = FLAG(12),
        //CastShadows = FLAG(10),
        //ReceiveShadows = FLAG(11),
    };

    typedef base::DirectFlags<EntityFlagBit> EntityFlags;
   
    //---

    // a basic element of runtime element hierarchy, elements can be linked together via runtime attachments to form complex arrangements
    // NOTE: this has 2 main derived classes: Component and Entity
    class GAME_SCENE_API Entity : public base::script::ScriptedObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Entity, base::script::ScriptedObject);

    public:
        Entity();
        Entity(const base::AbsoluteTransform& initialTransform);
        virtual ~Entity();

        //--

        /// get the world this entity is part of (NOTE: entity may NOT be yet attached)
        INLINE World* world() const { return m_world; }

        /// is the entity attached ?
        INLINE bool attached() const { return m_flags.test(EntityFlagBit::Attached); }

        /// get list of components in this entity
        INLINE const base::Array<ComponentPtr>& components() const { return m_components; }

        /// get absolute placement of the component
        INLINE const base::AbsoluteTransform& absoluteTransform() const { return m_absoluteTransform; }

        /// current entity placement (updated ONLY in handleTransformUpdate)
        INLINE const base::Matrix& localToWorld() const { return m_localToWorld; }

        /// get entity movement controller
        INLINE IEntityMovement* movement() const { return m_movement; }

        //--

        /// attach component to entity
        void attachComponent(const ComponentPtr& comp);

        /// detach component from entity
        void dettachComponent(const ComponentPtr& comp);

        //--

        /// change movement type of the entity
        virtual void movement(IEntityMovement* movement);

        //--

        /// update part before the systems are updated (good place to process input and send it to systems)
        virtual void handlePreTick(float dt);

        /// update part after the systems are updated (good place to suck data from systems)
        virtual void handlePostTick(float dt);

        /// update transform chain of this entity, called after source entities were updated (bucketed) so we can suck positions from them
        virtual void handleTransformUpdate();

        /// handle attachment to world, called during world update
        virtual void handleAttach(World* scene);

        /// handle detachment from scene
        virtual void handleDetach(World* scene);

        /// render debug elements
        virtual void handleDebugRender(rendering::scene::FrameParams& frame) const;

        /// handle game input event
        virtual void handleGameInput(const InputEventPtr& gameInput);

        //--

        /// get a world system, returns NULL if entity or is not attached
        template< typename T >
        INLINE T* system()
        {
            static auto userIndex = base::reflection::ClassID<T>()->userIndex();
            return (T*)systemPtr(userIndex);
        }

    private:
        World* m_world = nullptr;
        EntityFlags m_flags;

        //--

        base::AbsoluteTransform m_absoluteTransform; // absolute entity transform
        base::Matrix m_localToWorld; // computed during last transform update

        EntityMovementPtr m_movement; // current movement

        //--

        base::Array<ComponentPtr> m_components;

        //--

        void attachToWorld(World* world);
        void detachFromWorld(World* world);

        //--

        IWorldSystem* systemPtr(short index) const;

        //--

        friend class World;
    };

    ///---

} // game

