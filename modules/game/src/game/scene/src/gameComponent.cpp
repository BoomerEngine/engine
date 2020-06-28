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
#include "gameAttachment.h"
#include "gameEntity.h"
#include "gameTransformAttachment.h"

namespace game
{
    //--

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(Component);
    RTTI_END_TYPE();

    Component::Component()
        : m_entity(nullptr)
    {
        m_relativeScale = base::Vector3::ONE();
        m_relativePosition = base::Vector3::ZERO();
        m_relativeRotation = base::Quat::IDENTITY();
        m_localToWorld = base::Matrix::IDENTITY();
    }

    Component::~Component()
    {
        //DEBUG_CHECK_EX(!m_entity, "Destroying attached component");
        DEBUG_CHECK_EX(!attached(), "Destroying attached component");
    }

    //--

    void Component::relativePosition(const base::Vector3& pos)
    {
        if (m_relativePosition != pos)
        {
            m_relativePosition = pos;
            requestTransformUpdate();
        }
    }

    void Component::relativeRotation(const base::Quat& rot)
    {
        if (m_relativeRotation != rot)
        {
            m_relativeRotation = rot;
            requestTransformUpdate();
        }
    }

    void Component::relativeScale(const base::Vector3& scale)
    {
        if (m_relativeScale != scale)
        {
            m_relativeScale = scale;
            requestTransformUpdate();
        }
    }

    void Component::requestTransformUpdate()
    {
        
    }

    void Component::calcRelativeTransform(base::Transform& outTransform) const
    {
        outTransform.translation(m_relativePosition);
        outTransform.rotation(m_relativeRotation);
        outTransform.scale(m_relativeScale);
    }

    void Component::handleDebugRender(rendering::scene::FrameParams& frame) const
    {
        // TODO: draw component center 
    }

    void Component::handleTransformUpdate(const base::AbsoluteTransform& parentToWorld, Component* transformParentComponent)
    {
        base::Transform localToParent;
        calcRelativeTransform(localToParent);

        m_absoluteTransform = parentToWorld * localToParent; 
        m_localToWorld = m_absoluteTransform.approximate();

        for (const auto& att : m_transformOutgoingAttachments)
        {
            if (auto dest = att->destination().unsafe())
                dest->handleTransformUpdate(m_absoluteTransform, this);
        }
    }

    bool Component::enumerateAttachedComponents(ComponentEnumerationMode mode, const std::function<bool(Component*)>& enumFunc)
    {
        if (mode == ComponentEnumerationMode::Both || mode == ComponentEnumerationMode::Incoming)
        {
            for (const auto& att : m_incomingAttachments)
            {
                DEBUG_CHECK_EX(att->destination() == this, "Destination of incoming attachment does not point to this component");
                auto source = att->source().unsafe();
                DEBUG_CHECK_EX(source, "Source of incoming attachment has expired but attachment still exists");
                if (source)
                    if (enumFunc(source))
                        return true;
            }
        }

        if (mode == ComponentEnumerationMode::Both || mode == ComponentEnumerationMode::Outgoing || mode == ComponentEnumerationMode::OutgoingRecusrive)
        {
            for (const auto& att : m_outgoingAttachments)
            {
                DEBUG_CHECK_EX(att->source() == this, "Source of outgoing attachment does not point to this component");
                auto dest = att->destination().unsafe();
                DEBUG_CHECK_EX(dest, "Destination of outgoing attachment has expired but attachment still exists");
                if (dest)
                    if (enumFunc(dest))
                        return true;

                if (mode == ComponentEnumerationMode::OutgoingRecusrive)
                    if (dest->enumerateAttachedComponents(mode, enumFunc))
                        return true;
            }
        }

        return false;
    }

    void Component::breakAllAttachments()
    {}

    void Component::breakAllIncomingAttachments()
    {}

    void Component::breakAllOutgoingAttachments()
    {}

    bool Component::canAttachToSource(const Component* other, const IAttachment* att) const
    {
        return true;
    }

    bool Component::canAttachToDestination(const Component* other, const IAttachment* att) const
    {
        return true;
    }

    void Component::handleIncomingAttachmentBroken(IAttachment* att)
    {
        if (att == m_transformAttachment)
            m_transformAttachment = nullptr;
    }

    void Component::handleIncomingAttachmentCreated(IAttachment* att)
    {
        if (att->is<TransformAttachment>())
            m_transformAttachment = base::rtti_cast<TransformAttachment>(att);
    }

    void Component::handleOutgoingAttachmentBroken(IAttachment* att)
    {
        if (auto ta = base::rtti_cast<TransformAttachment>(att))
        {
            DEBUG_CHECK(m_transformOutgoingAttachments.contains(ta));
            m_transformOutgoingAttachments.remove(ta);
        }
    }

    void Component::handleOutgoingAttachmentCreated(IAttachment* att)
    {
        if (auto ta = base::rtti_cast<TransformAttachment>(att))
        {
            DEBUG_CHECK(!m_transformOutgoingAttachments.contains(ta));
            m_transformOutgoingAttachments.pushBackUnique(ta);
        }
    }

    IWorldSystem* Component::systemPtr(short index) const
    {
        if (attached() && m_entity && m_entity->world())
            return m_entity->world()->systemPtr(index);
        return nullptr;
    }

    void Component::detachFromWorld(World* world)
    {
        ASSERT(world);
        ASSERT(m_flags.test(ComponentFlagBit::Attached));
        ASSERT(!m_flags.test(ComponentFlagBit::Attaching));
        ASSERT(!m_flags.test(ComponentFlagBit::Detaching));
        m_flags |= ComponentFlagBit::Detaching;
        handleDetach(world);
        ASSERT(!m_flags.test(ComponentFlagBit::Detaching));
        m_flags -= ComponentFlagBit::Detaching;
    }

    void Component::attachToWorld(World* world)
    {
        ASSERT(world);
        ASSERT(!m_flags.test(ComponentFlagBit::Attaching));
        ASSERT(!m_flags.test(ComponentFlagBit::Detaching));
        m_flags |= ComponentFlagBit::Attaching;
        handleAttach(world);
        ASSERT(!m_flags.test(ComponentFlagBit::Attaching));
        ASSERT(m_flags.test(ComponentFlagBit::Attached));
        m_flags -= ComponentFlagBit::Attaching;
        m_flags |= ComponentFlagBit::Attached;
    }

    void Component::handleAttach(World* scene)
    {
        ASSERT(scene);
        ASSERT(m_flags.test(ComponentFlagBit::Attaching));
        m_flags -= ComponentFlagBit::Attaching;
        m_flags |= ComponentFlagBit::Attached;
    }

    void Component::handleDetach(World* scene)
    {
        ASSERT(scene);
        ASSERT(m_flags.test(ComponentFlagBit::Detaching));
        m_flags -= ComponentFlagBit::Detaching;
        m_flags -= ComponentFlagBit::Attached;
    }

    //--

    void Component::addOugoingAttachment(IAttachment* att)
    {
        DEBUG_CHECK(att != nullptr);
        DEBUG_CHECK(!m_outgoingAttachments.contains(att));
        DEBUG_CHECK(!m_incomingAttachments.contains(att));
        DEBUG_CHECK(att->source() == this);
        m_outgoingAttachments.pushBack(AddRef(att));
    }

    void Component::addIncomingAttachment(IAttachment* att)
    {
        DEBUG_CHECK(att != nullptr);
        DEBUG_CHECK(!m_outgoingAttachments.contains(att));
        DEBUG_CHECK(!m_incomingAttachments.contains(att));
        DEBUG_CHECK(att->destination() == this);
        m_incomingAttachments.pushBack(AddRef(att));
    }

    void Component::removeOugoingAttachment(IAttachment* att)
    {
        DEBUG_CHECK(att != nullptr);
        DEBUG_CHECK(m_outgoingAttachments.contains(att));
        DEBUG_CHECK(!m_incomingAttachments.contains(att));
        DEBUG_CHECK(att->source() == this);
        m_outgoingAttachments.remove(att);
    }

    void Component::removeIncomingAttachment(IAttachment* att)
    {
        DEBUG_CHECK(att != nullptr);
        DEBUG_CHECK(!m_outgoingAttachments.contains(att));
        DEBUG_CHECK(m_incomingAttachments.contains(att));
        DEBUG_CHECK(att->destination() == this);
        m_incomingAttachments.remove(att);
    }

    //--

} // game