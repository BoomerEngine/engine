/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading #]
***/

#include "build.h"
#include "multiQueue.h"

#ifdef PLATFORM_WINDOWS
    #include "multiQueueWindows.h"
#elif defined(PLATFORM_POSIX)
    #include "multiQueuePOSIX.h"
#endif

namespace base
{
    //---

    IMultiQueue::~IMultiQueue()
    {}

    IMultiQueue* IMultiQueue::Create(uint32_t numInternalQueues)
    {
#if defined(PLATFORM_WINDOWS)
        return prv::WinMultiQueue::Create(numInternalQueues);
#elif defined(PLATFORM_POSIX)
        return prv::POSIXMultiQueue::Create(numInternalQueues);
#else
        return nullptr; // no thread needed apparently :P
#endif
    }

    //---

} // inf