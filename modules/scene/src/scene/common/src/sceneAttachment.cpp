/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
*
***/

#include "build.h"
#include "sceneAttachment.h"
#include "sceneElement.h"

namespace scene
{
    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IAttachment);
    RTTI_END_TYPE();

    IAttachment::IAttachment()
    {}

    IAttachment::~IAttachment()
    {}

    void IAttachment::detach()
    {}

    bool IAttachment::attach(const ElementPtr& source, const ElementPtr& dest)
    {
        if (!m_source.empty() || !m_dest.empty())
            return false;

        if (!canAttach(source, dest))
            return false;

        auto self = sharedFromThis();

        if (!source->canAttachToDestination(dest, self))
            return false;
        if (!source->canAttachToSource(source, self))
            return false;

        m_source = source;
        m_dest = dest;

        source->m_outgoingAttachments.pushBack(self);
        dest->m_incomingAttachments.pushBack(self);

        source->handleIncomingAttachmentCreated(self);
        dest->handleOutgoingAttachmentCreated(self);

        return true;
    }

    bool IAttachment::canAttach(const ElementPtr& source, const ElementPtr& dest) const
    {
        return false;
    }

} // scene