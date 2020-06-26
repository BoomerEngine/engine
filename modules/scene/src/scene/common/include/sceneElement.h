/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
*
***/

#pragma once

#include "sceneAttachment.h"

namespace scene
{
    //---

    enum class ElementFlagBit : uint64_t
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

    typedef base::DirectFlags<ElementFlagBit> ElementFlags;

    //---

    // a basic element of runtime element hierarchy, elements can be linked together via runtime attachments to form complex arrangements
    // NOTE: this has 2 main derived classes: Component and Entity
    class SCENE_COMMON_API Element : public base::script::ScriptedObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Element, base::script::ScriptedObject);

    public:
        Element();
        virtual ~Element();

        /// get the scene we are attached to
        INLINE Scene* scene() const { return m_scene; }

        /// are we attached to scene ?
        INLINE bool isAttached() const { return m_scene; }

        /// get current element flags
        INLINE ElementFlags flags() const { return m_flags; }

        /// get current component relative position
        INLINE const base::Vector3& relativePosition() const { return m_relativePosition; }

        /// get current component relative rotation
        INLINE const base::Quat& relativeRotation() const { return m_relativeRotation; }

        /// get placement for component (computed in last update transform)
        INLINE const base::Matrix& localToWorld() const { return m_localToWorld; }

        /// get list of incoming attachments
        INLINE const base::Array<AttachmentPtr>& incomingAttachments() const { return m_incomingAttachments; }

        /// get list of outgoing attachments
        INLINE const base::Array<AttachmentPtr>& outgoingAttachments() const { return m_outgoingAttachments; }

        /// get transform attachment for this element, null only for "root transform" elements that are attached to "world"
        INLINE const TransformAttachmentPtr& transformAttachment() const { return m_transformAttachment; }

        ///---

        /// change component relative position (with respect to transform attachment)
        void relativePosition(const base::Vector3& pos);

        /// change component relative rotation (with respect to transform attachment)
        void relativeRotation(const base::Quat& rot);

        /// invalidate transform of this element, this will cause a call to "handleTransformUpdate" to be executed on this element and all child elements attached via transform attachments
        void requestTransformUpdate();

        /// handle a transform update for this element
        virtual void handleTransformUpdate(const base::Matrix& parentToWorld);

        /// calculate relative transform
        virtual void calcRelativeTransform(base::Matrix& outMatrix) const;

        /// render debug elements
        virtual void handleDebugRender(rendering::scene::FrameInfo& frame) const;

        ///---

        /// break all attachments of this components
        void breakAllAttachments();

        /// break all incoming attachments of this components
        void breakAllIncomingAttachments();

        /// break all outgoing attachments of this components
        void breakAllOutgoingAttachments();

        /// get all components attached to us
        void collectAttachedElements(base::Array<ElementPtr>& outElements) const;

        /// get all components attached to us via incoming attachments
        void collectAttachedIncomingElements(base::Array<ElementPtr>& outElements) const;

        /// get all components attached to us via outgoing attachments
        void collectAttachedOutgoingElements(base::Array<ElementPtr>& outElements) const;

        ///---

        /// check if we can be attached to other elements
        virtual bool canAttachToSource(const ElementPtr& other, const AttachmentPtr& att) const;

        /// check if we can be attached to other elements
        virtual bool canAttachToDestination(const ElementPtr& other, const AttachmentPtr& att) const;

        /// notification that attachment was broken
        virtual void handleIncomingAttachmentBroken(const AttachmentPtr& att);

        /// notification that attachment was created
        virtual void handleIncomingAttachmentCreated(const AttachmentPtr& att);

        /// notification that attachment was broken
        virtual void handleOutgoingAttachmentBroken(const AttachmentPtr& att);

        /// notification that attachment was created
        virtual void handleOutgoingAttachmentCreated(const AttachmentPtr& att);

        //---

        /// attach element to scene, fails if we are already attached
        void attachToScene(Scene* scene);

        /// detach element from scene
        void detachFromScene();

        /// handle attachment to scene
        virtual void handleSceneAttach(Scene* scene);

        /// handle detachment from scene
        virtual void handleSceneDetach(Scene* scene);

        //---

        /// set runtime flag
        void flag(ElementFlags flags);

        /// clear runtime flag
        void clearFlag(ElementFlags flags);

        /// runtime flags were changed
        virtual void handleFlagChange(ElementFlags prevFlags);

        //---

        /// calculate required streaming distance for the element
        virtual float calculateRequiredStreamingDistance() const;

    protected:
        Scene* m_scene;
        ElementFlags m_flags;

        base::Vector3 m_relativePosition;
        base::Quat m_relativeRotation;
        bool m_relativeTransformSet;

        base::Matrix m_localToWorld;

        TransformAttachmentPtr m_transformAttachment; // attachment that controls our position, incoming type 

        base::Array<AttachmentPtr> m_incomingAttachments; // attachments we are the destination
        base::Array<AttachmentPtr> m_outgoingAttachments; // attachments we are the source

        //--

        friend class IAttachment;
    };

    //---

} // scene
