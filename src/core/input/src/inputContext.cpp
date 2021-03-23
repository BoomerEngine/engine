/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
***/

#include "build.h"
#include "inputContext.h"

#if defined(PLATFORM_WINDOWS)
    #include "inputContextWinApi.h"
    typedef boomer::InputContextWinApi InputSystemClass;
#elif defined(PLATFORM_LINUX)
    #include "inputContextX11.h"
    typedef boomer::ContextX11 InputSystemClass;
#else
    #include "inputContextNull.h"
    typedef boomer::InputContextNull InputSystemClass;
#endif

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IInputContext);
RTTI_END_TYPE();

IInputContext::IInputContext()
{
}

void IInputContext::clear()
{
    auto lock = CreateLock(m_eventQueueLock);
    m_eventQueue.clear();
}

void IInputContext::inject(const EventPtr& evt)
{
    if (evt)
    {
        auto lock = CreateLock(m_eventQueueLock);
        m_eventQueue.push(evt);
    }
}

EventPtr IInputContext::pull()
{
    auto lock = CreateLock(m_eventQueueLock);

    if (m_eventQueue.empty())
        return nullptr;

    auto ret = m_eventQueue.top();
    m_eventQueue.pop();
    return ret;
}

ContextPtr IInputContext::CreateNativeContext(uint64_t nativeWindow, uint64_t nativeDisplay, bool gameMode)
{
    return RefNew<InputSystemClass>(nativeWindow, nativeDisplay, gameMode);
}

//---
        
END_BOOMER_NAMESPACE()
