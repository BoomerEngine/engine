/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading\winapi #]
* [#platform: windows #]
***/

#include "build.h"
#include "mutex.h"

namespace base
{
    Mutex::Mutex()
    {
        static_assert(sizeof(Mutex::m_data) >= sizeof(CRITICAL_SECTION), "Critical section data to small");
        InitializeCriticalSection((CRITICAL_SECTION*)&m_data);
    }

    Mutex::~Mutex()
    {
        DeleteCriticalSection((CRITICAL_SECTION*)&m_data);
    }

    void Mutex::acquire()
    {
        EnterCriticalSection((CRITICAL_SECTION*)&m_data);
    }

    void Mutex::release()
    {
        LeaveCriticalSection((CRITICAL_SECTION*)&m_data);
    }

    void Mutex::spinCount(uint32_t spinCount)
    {
        SetCriticalSectionSpinCount((CRITICAL_SECTION*)&m_data, spinCount);
    }

} // base
