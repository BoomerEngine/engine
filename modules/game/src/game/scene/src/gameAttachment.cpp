/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity #]
*
***/

#include "build.h"
#include "gameComponent.h"
#include "gameAttachment.h"

namespace game
{
    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IAttachment);
    RTTI_END_TYPE();

    IAttachment::IAttachment()
    {}

    IAttachment::~IAttachment()
    {}

    void IAttachment::detach()
    {
        if (auto dest = m_dest.lock())
            dest->removeIncomingAttachment(this);

        if (auto source = m_source.lock())
            source->removeOugoingAttachment(this);

        m_dest.reset();
        m_source.reset();
    }

    bool IAttachment::attach(Component* source, Component* dest)
    {
        if (!m_source.empty() || !m_dest.empty())
            return false;

        if (!canAttach(source, dest))
            return false;

        if (!source->canAttachToDestination(dest, this))
            return false;
        if (!source->canAttachToSource(source, this))
            return false;

        m_source = source;
        m_dest = dest;

        source->m_outgoingAttachments.emplaceBack(AddRef(this));
        dest->m_incomingAttachments.emplaceBack(AddRef(this));

        source->handleIncomingAttachmentCreated(this);
        dest->handleOutgoingAttachmentCreated(this);

        return true;
    }

    bool IAttachment::canAttach(const Component* source, const Component* dest) const
    {
        if (source == dest)
            return false;

        if (source->entity() != dest->entity())
            return false; // TODO: explore this avenue 



        return true;
    }

} // game