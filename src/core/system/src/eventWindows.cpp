/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading\winapi #]
* [#platform: windows #]
***/

#include "build.h"
#include "event.h"

BEGIN_BOOMER_NAMESPACE()

Event::Event(bool manualReset)
{
    m_event = CreateEvent(NULL, manualReset, 0, NULL);
}

Event::~Event()
{
    CloseHandle((HANDLE)m_event);
}

void Event::trigger()
{
    SetEvent((HANDLE)m_event);
}

void Event::reset()
{
    ResetEvent((HANDLE)m_event);
}

bool Event::wait(uint32_t waitTime)
{
    DEBUG_CHECK_EX(waitTime != INFINITE, "Invalid wait time");
    auto ret = WaitForSingleObject((HANDLE)m_event, waitTime);

    DEBUG_CHECK_EX(ret == WAIT_OBJECT_0 || ret == WAIT_TIMEOUT, "Event lost while waiting for it");
    return (ret == WAIT_OBJECT_0);
}

void Event::waitInfinite()
{
    auto ret = WaitForSingleObject((HANDLE)m_event, INFINITE);
    DEBUG_CHECK_EX(ret == WAIT_OBJECT_0, "Event lost while waiting for it");
}

END_BOOMER_NAMESPACE()
