/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity #]
*
***/

#pragma once

#include "core/script/include/scriptObject.h"

#if 0
        //---

        enum class ComponentFlagBit : uint64_t
        {
            HasRelativePosition = FLAG(0),
            HasRelativeRotation = FLAG(1),
            HasRelativeScale = FLAG(2),
            IdentityRelativeTransform = FLAG(3), // relative transform is identity
            Attaching = FLAG(10),
            Detaching = FLAG(11),
            Attached = FLAG(12),
            EffectSelectionHighlight = FLAG(13),
            //CastShadows = FLAG(10),
            //ReceiveShadows = FLAG(11),
        };

        typedef DirectFlags<ComponentFlagBit> ComponentFlags;

        //---

        enum class ComponentEnumerationMode : uint8_t
        {
            Both,
            Incoming,
            Outgoing,
            OutgoingRecusrive,
        };

        //---

        struct ComponentUpdateContext
        {
            const Component** incomingDependencies = nullptr;
            const Component** outgoingDependencies = nullptr;
            uint32_t numIncomingLinks = 0;
            uint32_t numOutgoingLinks = 0;
        };

        //---

        // link between two components
        class IComponentLink : public IObject
        {
            RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
            RTTI_DECLARE_VIRTUAL_CLASS(IComponentLink, IObject);

        public:
            IComponentLink(Component* source, Component* target);

            //--

            // get source component (may be gone)
            INLINE Component* source() const { return m_source.unsafe(); }

            // get target component (may be gone)
            INLINE Component* target() const { return m_target.unsafe(); }

            // is the link active ?
            INLINE bool linked() const { return m_linked; }

            //--

            // link components, may fail
            bool link();

            // unlink components
            void unlink();

            //--
        
        private:
            ComponentWeakPtr m_source;
            ComponentWeakPtr m_target;
            bool m_linked = false;
        };

        //---

        // transform link between two components
        class ComponentTransformLink : public IComponentLink
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ComponentTransformLink, IComponentLink);

        public:
            ComponentTransformLink(Component* source, Component* target);

            // calculate new absolute transform for component
            virtual void calculateNewAbsoluteTransform(const Transform& componentRelativeTransform, ComponentFlags componentFlags, const Transform& parentTransform, const Matrix& parentToWorld,   Transform& outNewAbsoluteTransform, Matrix& outNewLocalToWorldMatrix) const;
        };

        //---

        // transform link between two components with extra options
        class ComponentTransformLinkEx : public ComponentTransformLink
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ComponentTransformLinkEx, ComponentTransformLink);

        public:
            ComponentTransformLinkEx(Component* source, Component* target, const Transform& relativeTransform, bool updateRotation, bool updateScale);

            // calculate new absolute transform for component
            virtual void calculateNewAbsoluteTransform(const Transform& componentRelativeTransform, ComponentFlags componentFlags, const Transform& parentTransform, const Matrix& parentToWorld, Transform& outNewAbsoluteTransform, Matrix& outNewLocalToWorldMatrix) const override final;

        private:
            Transform m_additionalTransform;
            bool m_updateRotation = true;
            bool m_updateScale = true;
        };

        //---

        // a basic runtime component
        class ENGINE_WORLD_API Component : public script::ScriptedObject
        {
            RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
            RTTI_DECLARE_VIRTUAL_CLASS(Component, script::ScriptedObject);

        public:
            Component();
            virtual ~Component();

            //--

            /// get entity that this component belongs to, world can be quiered from entity()->world()
            INLINE Entity* entity() const { return m_entity; }

            /// get name of the component
            INLINE StringID name() const { return m_name; }

            /// get current relative transform to the components "transform parent" (either other component or entity)
            INLINE const Transform& relativeTransform() const { return m_relativeTransform; }

            /// get absolute placement of the component
            INLINE const Transform& absoluteTransform() const { return m_absoluteTransform; }

            /// get matrix representing placement of the component in the world
            INLINE const Matrix& localToWorld() const { return m_localToWorld; }

            /// transform link for this component (points to component that we based our transform on)
            INLINE const ComponentTransformLink* transformLink() const { return m_links ? m_links->m_transformLink : nullptr; }

            /// are we attached to the world ?
            INLINE bool attached() const { return m_flags.test(ComponentFlagBit::Attached); }

            /// should we rendering with selection highlight ?
            INLINE bool selected() const { return m_flags.test(ComponentFlagBit::EffectSelectionHighlight); }

            //--

            /// bind name to component, possible only if not part of entity yet
            void bindName(StringID name);

            //--

            /// request entity transform hierarchy update (including this component)
            void requestTransformUpdate();

            // update transform of this component, called after it's transform parent is updated or directly from entity if component has no parent transform
            virtual void handleTransformUpdate(const Component* source, const Transform& parentTransform, const Matrix& parentToWorld);

            //--

            /// change component relative position (with respect to transform attachment)
            /// NOTE: will take effect only after next transform update
            void relativePosition(const Vector3& pos);

            /// change component relative rotation (with respect to transform attachment)
            /// NOTE: will take effect only after next transform update
            void relativeRotation(const Quat& rot);

            /// change component relative scale (with respect to transform attachment)
            /// NOTE: will take effect only after next transform update
            void relativeScale(const Vector3& scale);

            /// change whole relative transform
            void relativeTransform(const Transform& newTransform);

            ///---

            /// unlink current transform, will return component to the entity "realm"
            ComponentTransformLinkPtr unlinkTransform(bool preserveWorldPosition = true);

            /// link transform with another component, current world position can be preserve if we wish (ideal for sticking "arrows into the body")
            /// NOTE: breaks existing transform connection
            ComponentTransformLinkPtr linkTransform(Component* transformSource, bool preserveWorldPosition = true);

            /// link transform with another component, more options
            ComponentTransformLinkPtr linkTransformEx(Component* transformSource, Transform& relativeTransform, bool updateRotation=true, bool updateScale=true);

            //---

            /// remove all links that have given component as destination
            void breakIncomingLinks();

            /// remove all links that have given component as source
            void breakOutgoingLinks();

            /// break all links between components within this entity
            void breakAllLinks();

            //---

            /// handle attachment to world
            virtual void handleAttach(World* world);

            /// handle detachment from world
            virtual void handleDetach(World* world);

            /// an incoming link was created for this component, return false to VETO
            virtual bool handleIncomingLinkCreated(Component* sourceComponent, IComponentLink* link);

            /// an outgoing link was created for this component, return false to VETO
            virtual bool handleOutgoingLinkCreated(Component* targetComponent, IComponentLink* link);

            /// an incoming link was removed from this component
            virtual void handleIncomingLinkDestroyed(Component* sourceComponent, IComponentLink* link);

            /// an outgoing link was removed from this component
            virtual void handleOutgoingLinkDestroyed(Component* targetComponent, IComponentLink* link);

            //--

            /// render debug elements
            virtual void handleDebugRender(rendering::FrameParams& frame) const;

            //--

            /// toggle selection highlight
            void toggleSelectionHighlight(bool flag);

            /// sync the selection highlight effect with rendering proxy
            virtual void handleSelectionHighlightChanged();

            //---

            /// calculate component bounds, usually using the visuals
            virtual Box calcBounds() const;

            /// calculate streaming bounds for the component
            virtual Box calcStreamingBounds() const;

            //---

            // determine final component class
            virtual SpecificClassType<Component> determineComponentTemplateClass(const ITemplatePropertyValueContainer& templateProperties);

            // list template properties for the entity
            virtual void queryTemplateProperties(ITemplatePropertyBuilder& outTemplateProperties) const override;

            // initialize entity from template properties
            virtual bool initializeFromTemplateProperties(const ITemplatePropertyValueContainer& templateProperties) override;

            //---

            /// get a world system, returns NULL if component has no entity or is not attached
            template< typename T >
            INLINE T* system()
            {
                static auto userIndex = ClassID<T>()->userIndex();
                return (T*)systemPtr(userIndex);
            }

        protected:
            // shorhand for entity()->world()
            World* world() const;

            // shorhand for entity()->world()->system()
            IWorldSystem* systemPtr(short index) const;

        private:
            StringID m_name;
            ComponentFlags m_flags;
            Entity* m_entity = nullptr;

            //--

            Transform m_relativeTransform;
            Transform m_absoluteTransform;
            Matrix m_localToWorld;

            //--

            struct LinksData
            {
                ComponentTransformLink* m_transformLink = nullptr; // most common case
                Array<ComponentTransformLink*> m_outgoingTransformLinks;

                Array<ComponentLinkPtr> m_incomingLinks;
                Array<ComponentLinkPtr> m_outgoingLinks;
            };

            UniquePtr<LinksData> m_links;

            //--

            friend class Entity;
            friend class IComponentLink;

            static bool LinkComponents(const ComponentWeakPtr& source, const ComponentWeakPtr& target, IComponentLink* link);
            static void UnlinkComponents(const ComponentWeakPtr& source, const ComponentWeakPtr& target, IComponentLink* link);

            void detachFromWorld(World* scene);
            void attachToWorld(World* scene);

            void recalculateTransform(const Transform& parentTransform, const Matrix& parentToWorld);

            void updateRelativeTransformFlags();
        };

        //---
#endif
