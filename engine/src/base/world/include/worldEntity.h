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

namespace base
{
    namespace world
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

        typedef DirectFlags<EntityFlagBit> EntityFlags;

        //---

        /// generic input event
        class BASE_WORLD_API EntityInputEvent : public IObject
        {
            RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
            RTTI_DECLARE_VIRTUAL_CLASS(EntityInputEvent, IObject);

        public:
            StringID m_name;
            float m_deltaValue = 0.0f;
            float m_absoluteValue = 0.0f;

            EntityInputEvent();
        };

        //---

        // a basic game dweller, contains components and links between them
        class BASE_WORLD_API Entity : public script::ScriptedObject
        {
            RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
            RTTI_DECLARE_VIRTUAL_CLASS(Entity, script::ScriptedObject);

        public:
            Entity();
            virtual ~Entity();

            //--

            /// get the world this entity is part of (NOTE: entity may NOT be yet attached)
            INLINE World* world() const { return m_world; }

            /// is the entity attached ?
            INLINE bool attached() const { return m_flags.test(EntityFlagBit::Attached); }

            /// get list of components in this entity
            INLINE const Array<ComponentPtr>& components() const { return m_components; }

            /// get absolute placement of the component
            /// NOTE: look at World::moveEntity() for a way to move entity to different place
            INLINE const AbsoluteTransform& absoluteTransform() const { return m_absoluteTransform; }

            /// current entity placement (updated ONLY in handleTransformUpdate)
            INLINE const Matrix& localToWorld() const { return m_localToWorld; }

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
            void requestTransform(const AbsoluteTransform& newTransform);

            /// request entity to be moved to new location keeping existing rotation/scale
            /// NOTE: for non attached entities this moves the entity right away, for attached entities this asks WORLD to move the entity
            void requestMove(const AbsolutePosition& newPosition);

            /// request entity to be moved to new location/orientation but keeping existing scale
            /// NOTE: for non attached entities this moves the entity right away, for attached entities this asks WORLD to move the entity
            void requestMove(const AbsolutePosition& newPosition, const Quat& newRotation);

            //--

            /// update part before the systems are updated (good place to process input and send it to systems)
            virtual void handlePreTick(float dt);

            /// update part after the systems are updated (good place to suck data from systems)
            virtual void handlePostTick(float dt);

            /// update transform chain of this entity, NOTE: can be called even if we don't have world attached (it will basically dry-move entity to new place)
            virtual void handleTransformUpdate(const AbsoluteTransform& transform);

            /// handle attachment to world, called during world update
            virtual void handleAttach(World* scene);

            /// handle detachment from scene
            virtual void handleDetach(World* scene);

            /// render debug elements
            virtual void handleDebugRender(rendering::scene::FrameParams& frame) const;

            /// handle game input event
            virtual void handleInput(const EntityInputEvent& gameInput);

            //--

            /// get a world system, returns NULL if entity or is not attached
            template< typename T >
            INLINE T* system()
            {
                static auto userIndex = reflection::ClassID<T>()->userIndex();
                return (T*)systemPtr(userIndex);
            }

            //---

            /// calculate entity bounds using visible components
            Box calcBounds(bool includeEntityCenter=false) const;

            //---

            // determine final entity class
            virtual SpecificClassType<Entity> determineEntityTemplateClass(const ITemplatePropertyValueContainer& templateProperties);

            // list template properties for the entity
            virtual void queryTemplateProperties(ITemplatePropertyBuilder& outTemplateProperties) const override;

            // initialize entity from template properties
            virtual bool initializeFromTemplateProperties(const ITemplatePropertyValueContainer& templateProperties) override;

        private:
            World* m_world = nullptr;
            EntityFlags m_flags;

            //--

            AbsoluteTransform m_absoluteTransform; // absolute entity transform
            Matrix m_localToWorld; // computed during last transform update

            //--

            Array<ComponentPtr> m_components; // all components, always ordered by the attachment order
            Array<Component*> m_rootComponents; // components with no parent transforms


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

    } // world
} // base

