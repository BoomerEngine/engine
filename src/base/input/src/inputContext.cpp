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
    typedef base::input::ContextWinApi InputSystemClass;
#elif defined(PLATFORM_LINUX)
    #include "inputContextX11.h"
    typedef base::input::ContextX11 InputSystemClass;
#else
    #include "inputContextNull.h"
    typedef base::input::ContextNull InputSystemClass;
#endif

BEGIN_BOOMER_NAMESPACE(base::input)

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IContext);
RTTI_END_TYPE();

IContext::IContext()
{
}

void IContext::clear()
{
    auto lock = CreateLock(m_eventQueueLock);
    m_eventQueue.clear();
}

void IContext::inject(const EventPtr& evt)
{
    if (evt)
    {
        auto lock = CreateLock(m_eventQueueLock);
        m_eventQueue.push(evt);
    }
}

EventPtr IContext::pull()
{
    auto lock = CreateLock(m_eventQueueLock);

    if (m_eventQueue.empty())
        return nullptr;

    auto ret = m_eventQueue.top();
    m_eventQueue.pop();
    return ret;
}

ContextPtr IContext::CreateNativeContext(uint64_t nativeWindow, uint64_t nativeDisplay, bool gameMode)
{
    return base::RefNew<InputSystemClass>(nativeWindow, nativeDisplay, gameMode);
}

//---
        
END_BOOMER_NAMESPACE(base::input)