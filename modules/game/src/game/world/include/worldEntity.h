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
        DirtyTransform = FLAG(13),
        DirtyRoots = FLAG(14),
        //CastShadows = FLAG(10),
        //ReceiveShadows = FLAG(11),
    };

    typedef base::DirectFlags<EntityFlagBit> EntityFlags;
   
    //---

    // a basic game dweller, contains components and links between them
    class GAME_WORLD_API Entity : public base::script::ScriptedObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Entity, base::script::ScriptedObject);

    public:
        Entity();
        virtual ~Entity();

        //--

        /// get the world this entity is part of (NOTE: entity may NOT be yet attached)
        INLINE World* world() const { return m_world; }

        /// is the entity attached ?
        INLINE bool attached() const { return m_flags.test(EntityFlagBit::Attached); }

        /// get list of components in this entity
        INLINE const base::Array<ComponentPtr>& components() const { return m_components; }

        /// get absolute placement of the component
        /// NOTE: look at World::moveEntity() for a way to move entity to different place
        INLINE const base::AbsoluteTransform& absoluteTransform() const { return m_absoluteTransform; }

        /// current entity placement (updated ONLY in handleTransformUpdate)
        INLINE const base::Matrix& localToWorld() const { return m_localToWorld; }

        //--

        /// attach component to entity
        void attachComponent(Component* comp);

        /// detach component from entity
        void dettachComponent(Component* comp);

        //--

        /// request hierarchy of this entity to be updated
        void requestTransformUpdate();

        /// request entity to be moved to new location
        /// NOTE: for non attached entities this moves the entity right away, for attached entities this asks WORLD to move the entity
        void requestTransform(const base::AbsoluteTransform& newTransform);

        /// request entity to be moved to new location keeping existing rotation/scale
        /// NOTE: for non attached entities this moves the entity right away, for attached entities this asks WORLD to move the entity
        void requestMove(const base::AbsolutePosition& newPosition);

        /// request entity to be moved to new location/orientation but keeping existing scale
        /// NOTE: for non attached entities this moves the entity right away, for attached entities this asks WORLD to move the entity
        void requestMove(const base::AbsolutePosition& newPosition, const base::Quat& newRotation);

        //--

        /// update part before the systems are updated (good place to process input and send it to systems)
        virtual void handlePreTick(float dt);

        /// update part after the systems are updated (good place to suck data from systems)
        virtual void handlePostTick(float dt);

        /// update transform chain of this entity, NOTE: can be called even if we don't have world attached (it will basically dry-move entity to new place)
        virtual void handleTransformUpdate(const base::AbsoluteTransform& transform);

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

        //--

        base::Array<ComponentPtr> m_components; // all components, always ordered by the attachment order
        base::Array<Component*> m_rootComponents; // components with no parent transforms


        //--

        void attachToWorld(World* world);
        void detachFromWorld(World* world);

        void invalidateRoots();
        bool isRootComponent(const Component* comp) const;

        //--

        IWorldSystem* systemPtr(short index) const;

        //--

        friend class World;
        friend class Component;
    };

    ///---

} // game

