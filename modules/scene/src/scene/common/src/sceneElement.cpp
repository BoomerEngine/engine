/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
*
***/

#include "build.h"

#include "sceneElement.h"
#include "sceneTransformAttachment.h"

namespace scene
{
    //--

    RTTI_BEGIN_TYPE_CLASS(Element);
    RTTI_END_TYPE();

    Element::Element()
        : m_scene(nullptr)
        , m_relativeTransformSet(false)
    {
        m_localToWorld.identity();
    }

    Element::~Element()
    {
        ASSERT(!m_scene);
    }

    void Element::relativePosition(const base::Vector3& pos)
    {
        if (m_relativePosition != pos)
        {
            m_relativePosition = pos;
            requestTransformUpdate();
        }
    }

    void Element::relativeRotation(const base::Quat& rot)
    {
        if (m_relativeRotation != rot)
        {
            m_relativeRotation = rot;
            requestTransformUpdate();
        }
    }

    void Element::requestTransformUpdate()
    {
        m_relativeTransformSet = !m_relativePosition.isZero() || m_relativeRotation != base::Quat::IDENTITY();
    }

    void Element::calcRelativeTransform(base::Matrix& outMatrix) const
    {
        if (m_relativeTransformSet)
        {
            outMatrix = m_relativeRotation.toMatrix();
            outMatrix.scaleTranslation(m_relativePosition);
        }
        else
        {
            outMatrix.identity();
        }
    }

    void Element::handleDebugRender(rendering::scene::FrameInfo& frame) const
    {

    }

    void Element::handleTransformUpdate(const base::Matrix& parentToWorld)
    {
        if (m_relativeTransformSet)
        {
            base::Matrix localToParent;
            calcRelativeTransform(localToParent);

            m_localToWorld = localToParent * parentToWorld;
        }
        else
        {
            m_localToWorld = parentToWorld;
        }
    }

    void Element::breakAllAttachments()
    {}

    void Element::breakAllIncomingAttachments()
    {}

    void Element::breakAllOutgoingAttachments()
    {}

    void Element::collectAttachedElements(base::Array<ElementPtr>& outElements) const
    {}

    void Element::collectAttachedIncomingElements(base::Array<ElementPtr>& outElements) const
    {}

    void Element::collectAttachedOutgoingElements(base::Array<ElementPtr>& outElements) const
    {}

    bool Element::canAttachToSource(const ElementPtr& other, const AttachmentPtr& att) const
    {
        return true;
    }

    bool Element::canAttachToDestination(const ElementPtr& other, const AttachmentPtr& att) const
    {
        return true;
    }

    void Element::handleIncomingAttachmentBroken(const AttachmentPtr& att)
    {
        if (att == m_transformAttachment)
            m_transformAttachment.reset();
    }

    void Element::handleIncomingAttachmentCreated(const AttachmentPtr& att)
    {
        if (att->is<TransformAttachment>())
            m_transformAttachment = base::rtti_cast<TransformAttachment>(att);
    }

    void Element::handleOutgoingAttachmentBroken(const AttachmentPtr& att)
    {

    }

    void Element::handleOutgoingAttachmentCreated(const AttachmentPtr& att)
    {

    }

    void Element::flag(ElementFlags flags)
    {
        auto oldFlags = m_flags;
        m_flags |= flags;
        if (m_flags != oldFlags)
            handleFlagChange(oldFlags);
    }

    void Element::clearFlag(ElementFlags flags)
    {
        auto oldFlags = m_flags;
        m_flags -= flags;
        if (m_flags != oldFlags)
            handleFlagChange(oldFlags);
    }

    void Element::handleFlagChange(ElementFlags prevFlags)
    {

    }

    void Element::attachToScene(Scene* scene)
    {
        ASSERT(!m_scene);
        ASSERT(scene);
        ASSERT(!m_flags.test(ElementFlagBit::Attaching));
        ASSERT(!m_flags.test(ElementFlagBit::Detaching));
        m_scene = scene;
        m_flags |= ElementFlagBit::Attaching;
        handleSceneAttach(scene);
        ASSERT(!m_flags.test(ElementFlagBit::Attaching));
        ASSERT(m_flags.test(ElementFlagBit::Attached));
        m_flags -= ElementFlagBit::Attaching;
        m_flags |= ElementFlagBit::Attached;
    }

    void Element::detachFromScene()
    {
        ASSERT(m_scene);
        ASSERT(m_flags.test(ElementFlagBit::Attached));
        ASSERT(!m_flags.test(ElementFlagBit::Attaching));
        ASSERT(!m_flags.test(ElementFlagBit::Detaching));
        m_flags |= ElementFlagBit::Detaching;
        handleSceneDetach(m_scene);
        ASSERT(!m_flags.test(ElementFlagBit::Detaching));
        m_flags -= ElementFlagBit::Detaching;
        m_scene = nullptr;
    }

    void Element::handleSceneAttach(Scene* scene)
    {
        ASSERT(scene);
        ASSERT(m_flags.test(ElementFlagBit::Attaching));
        m_flags -= ElementFlagBit::Attaching;
        m_flags |= ElementFlagBit::Attached;
    }

    void Element::handleSceneDetach(Scene* scene)
    {
        ASSERT(scene);
        ASSERT(m_flags.test(ElementFlagBit::Detaching));
        m_flags -= ElementFlagBit::Detaching;
        m_flags -= ElementFlagBit::Attached;
    }

    float Element::calculateRequiredStreamingDistance() const
    {
        return 0.0f;
    }

    //--

} // scen