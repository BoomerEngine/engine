/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "platform.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IGamePlatform);
RTTI_END_TYPE();

IGamePlatform::IGamePlatform()
{}

IGamePlatform::~IGamePlatform()
{}

void IGamePlatform::pushEvent(IGamePlatformEvent* evt)
{
    DEBUG_CHECK_RETURN_EX(evt, "Invalid event");

    auto lock = CreateLock(m_eventLock);
    m_events.push(AddRef(evt));
}

GamePlatformEventPtr IGamePlatform::popEvent()
{
    auto lock = CreateLock(m_eventLock);
    if (m_events.empty())
        return nullptr;

    auto ret = m_events.top();
    m_events.pop();
    return ret;
}

//--
    
END_BOOMER_NAMESPACE()
