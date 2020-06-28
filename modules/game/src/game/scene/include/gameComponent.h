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

    enum class ComponentFlagBit : uint64_t
    {
        Attaching = FLAG(10),
        Detaching = FLAG(11),
        Attached = FLAG(12),
        //CastShadows = FLAG(10),
        //ReceiveShadows = FLAG(11),
    };

    typedef base::DirectFlags<ComponentFlagBit> ComponentFlags;

    //---

    enum class ComponentEnumerationMode : uint8_t
    {
        Both,
        Incoming,
        Outgoing,
        OutgoingRecusrive,
    };

    //---

    // a basic runtime component
    class GAME_SCENE_API Component : public base::script::ScriptedObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Component, base::script::ScriptedObject);

    public:
        Component();
        virtual ~Component();

        //--

        /// get entity that this component belongs to, world can be quiered from entity()->world()
        INLINE Entity* entity() const { return m_entity; }

        /// get current relative position to it's parent, usually that parent is NULL (entity) or another component
        INLINE const base::Vector3& relativePosition() const { return m_relativePosition; }

        /// get current relative rotation
        INLINE const base::Quat& relativeRotation() const { return m_relativeRotation; }

        /// get current relative scale
        INLINE const base::Vector3& relativeScale() const { return m_relativeScale; }

        /// get absolute placement of the component
        INLINE const base::AbsoluteTransform& absoluteTransform() const { return m_absoluteTransform; }

        /// get matrix representing placement of the component in the world
        INLINE const base::Matrix& localToWorld() const { return m_localToWorld; }

        /// are we attached to the world ?
        INLINE bool attached() const { return m_flags.test(ComponentFlagBit::Attached); }

        //--

        /// get list of incoming attachments
        INLINE const base::Array<AttachmentPtr>& incomingAttachments() const { return m_incomingAttachments; }

        /// get list of outgoing attachments
        INLINE const base::Array<AttachmentPtr>& outgoingAttachments() const { return m_outgoingAttachments; }

        /// get transform attachment for this element, null only for "root transform" elements that are attached to "world"
        INLINE TransformAttachment* transformAttachment() const { return m_transformAttachment; }

        //--

        // update transform of this component, called after it's transform parent is updated
        // NOTE: transform parent component can be NULL (in case of being directly parented to entity) or ca be from another entity!
        virtual void handleTransformUpdate(const base::AbsoluteTransform& parentToWorld, Component* transformParentComponent);

        //--

        /// change component relative position (with respect to transform attachment)
        /// NOTE: will take effect only after next transform update
        void relativePosition(const base::Vector3& pos);

        /// change component relative rotation (with respect to transform attachment)
        /// NOTE: will take effect only after next transform update
        void relativeRotation(const base::Quat& rot);

        /// change component relative scale (with respect to transform attachment)
        /// NOTE: will take effect only after next transform update
        void relativeScale(const base::Vector3& scale);

        /// invalidate transform of this component, this will cause a call to "updateTransform" to be executed on this element and all child elements attached via transform attachments
        void requestTransformUpdate();

        /// calculate relative transform of this component (basically assembles matrix from the relativeXXX fields)
        virtual void calcRelativeTransform(base::Transform& outTransform) const;

        ///---

        /// break all attachments of this components
        void breakAllAttachments();

        /// break all incoming attachments of this components
        void breakAllIncomingAttachments();

        /// break all outgoing attachments of this components
        void breakAllOutgoingAttachments();

        ///--

        /// enumerate all components attached to us
        bool enumerateAttachedComponents(ComponentEnumerationMode mode, const std::function<bool(Component*)>& enumFunc);

        ///---

        /// check if we can be attached to other elements
        virtual bool canAttachToSource(const Component* other, const IAttachment* att) const;

        /// check if we can be attached to other elements
        virtual bool canAttachToDestination(const Component* other, const IAttachment* att) const;

        /// notification that attachment was broken
        virtual void handleIncomingAttachmentBroken(IAttachment* att);

        /// notification that attachment was created
        virtual void handleIncomingAttachmentCreated(IAttachment* att);

        /// notification that attachment was broken
        virtual void handleOutgoingAttachmentBroken(IAttachment* att);

        /// notification that attachment was created
        virtual void handleOutgoingAttachmentCreated(IAttachment* att);

        //---

        /// handle attachment to world
        virtual void handleAttach(World* world);

        /// handle detachment from world
        virtual void handleDetach(World* world);

        //--

        /// render debug elements
        virtual void handleDebugRender(rendering::scene::FrameParams& frame) const;

        //--

        /// get a world system, returns NULL if component has no entity or is not attached
        template< typename T >
        INLINE T* system()
        {
            static auto userIndex = base::reflection::ClassID<T>()->userIndex();
            return (T*)systemPtr(userIndex);
        }

    protected:
        Entity* m_entity = nullptr;
        ComponentFlags m_flags;

        //--

        base::Vector3 m_relativePosition;
        base::Vector3 m_relativeScale;
        base::Quat m_relativeRotation;

        base::AbsoluteTransform m_absoluteTransform;
        base::Matrix m_localToWorld;

        //--

        TransformAttachment* m_transformAttachment = nullptr; // attachment that controls our position, incoming type 
        base::Array<TransformAttachment*> m_transformOutgoingAttachments; // attachments we are the source

        base::Array<AttachmentPtr> m_incomingAttachments; // attachments we are the destination
        base::Array<AttachmentPtr> m_outgoingAttachments; // attachments we are the source

        //--

        friend class IAttachment;
        friend class Entity;

        void detachFromWorld(World* scene);
        void attachToWorld(World* scene);

        void addOugoingAttachment(IAttachment* att);
        void addIncomingAttachment(IAttachment* att);

        void removeOugoingAttachment(IAttachment* att);
        void removeIncomingAttachment(IAttachment* att);

        //--

        IWorldSystem* systemPtr(short index) const;
    };

    //---

} // game
